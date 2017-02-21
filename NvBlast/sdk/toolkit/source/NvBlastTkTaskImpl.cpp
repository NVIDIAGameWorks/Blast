/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NvBlastTime.h"

#include "NvBlastTkTaskImpl.h"
#include "NvBlastTkFamilyImpl.h"
#include "NvBlastTkAssetImpl.h"
#include "NvBlastTkGroupImpl.h"


using namespace Nv::Blast;


void SharedMemory::allocate(TkFamilyImpl& tkFamily)
{
	NVBLAST_ASSERT(m_refCount == 0);
	const NvBlastAsset* assetLL = tkFamily.getAsset()->getAssetLL();
	
	// at most leafChunkCount actors can be created within a family
	// tasks will grab their portion out of these memory blocks
	uint32_t leafChunkCount = NvBlastAssetGetLeafChunkCount(assetLL, TkFrameworkImpl::get()->log);
	m_newActorBuffers.allocate(2 * leafChunkCount); // GWD-167 workaround (2*)
	m_newTkActorBuffers.allocate(leafChunkCount);
}


/**
Creates a TkEvent::FractureCommand according to the input buffer for tkActor
into events queue using the LocalBuffers to store the actual event data.
*/
NV_FORCE_INLINE void reportFractureCommands(
	const NvBlastFractureBuffers& buffer,
	LocalBuffer<NvBlastBondFractureData>& bondBuffer, LocalBuffer<NvBlastChunkFractureData>& chunkBuffer,
	TkEventQueue& events, const TkActorImpl* tkActor)
{

	NvBlastBondFractureData* bdata = nullptr;
	if (buffer.bondFractureCount > 0)
	{
		bdata = bondBuffer.allocate(buffer.bondFractureCount);
		memcpy(bdata, buffer.bondFractures, sizeof(NvBlastBondFractureData)*buffer.bondFractureCount);
	}

	NvBlastChunkFractureData* cdata = nullptr;
	if (buffer.chunkFractureCount > 0)
	{
		cdata = chunkBuffer.allocate(buffer.chunkFractureCount);
		memcpy(cdata, buffer.chunkFractures, sizeof(NvBlastChunkFractureData)*buffer.chunkFractureCount);
	}

	TkFractureCommands* fevt = events.allocData<TkFractureCommands>();
	fevt->tkActorData = *tkActor;
	fevt->buffers = { buffer.bondFractureCount, buffer.chunkFractureCount, bdata, cdata };
	events.addEvent(fevt);
}


/**
Creates a TkEvent::FractureEvent according to the input buffer for tkActor
into events queue using the LocalBuffers to store the actual event data.
*/
NV_FORCE_INLINE void reportFractureEvents(
	const NvBlastFractureBuffers& buffer,
	LocalBuffer<NvBlastBondFractureData>& bondBuffer, LocalBuffer<NvBlastChunkFractureData>& chunkBuffer,
	TkEventQueue& events, const TkActorImpl* tkActor)
{
	uint32_t result[4] = { 0,0,0,0 };

	NvBlastBondFractureData* bdata = nullptr;
	if (buffer.bondFractureCount > 0)
	{
		bdata = bondBuffer.allocate(buffer.bondFractureCount);
		for (uint32_t b = 0; b < buffer.bondFractureCount; ++b)
		{
			bdata[b] = buffer.bondFractures[b];
			result[buffer.bondFractures[b].health > 0 ? 0 : 1]++;
		}
	}

	NvBlastChunkFractureData* cdata = nullptr;
	if (buffer.chunkFractureCount > 0)
	{
		cdata = chunkBuffer.allocate(buffer.chunkFractureCount);
		for (uint32_t c = 0; c < buffer.chunkFractureCount; ++c)
		{
			cdata[c] = buffer.chunkFractures[c];
			result[buffer.chunkFractures[c].health > 0 ? 2 : 3]++;
		}
	}

	TkFractureEvents* fevt = events.allocData<TkFractureEvents>();
	fevt->tkActorData = *tkActor;
	fevt->buffers = { buffer.bondFractureCount, buffer.chunkFractureCount, bdata, cdata };
	fevt->bondsDamaged = result[0];
	fevt->bondsBroken = result[1];
	fevt->chunksDamaged = result[2];
	fevt->chunksBroken = result[3];
	events.addEvent(fevt);
}


void TkWorker::run()
{
	PERF_SCOPE_L("TkWorker Task");

	NvBlastTimers* timers = nullptr;

#if NV_PROFILE
	NvBlastTimers myTimers;
	timers = &myTimers;
	NvBlastTimersReset(timers);
	uint32_t jobCount = 0;
	Time workTime;
#endif

	// temporary memory used to generate and apply fractures
	// it must fit for the largest family involved in the group that owns this worker 
	NvBlastBondFractureData* bondFractureData = m_group->m_bondTempDataBlock.getBlock(m_id);
	uint32_t bondFractureCount = m_group->m_bondTempDataBlock.numElementsPerBlock();
	NvBlastChunkFractureData* chunkFractureData = m_group->m_chunkTempDataBlock.getBlock(m_id);
	uint32_t chunkFractureCount = m_group->m_chunkTempDataBlock.numElementsPerBlock();
	const NvBlastFractureBuffers tempBuffer = { bondFractureCount, chunkFractureCount, bondFractureData, chunkFractureData };

	// temporary memory used to split the actor
	// large enough for the largest family involved
	void* splitScratch = m_group->m_splitScratchBlock.getBlock(m_id);

	// to avoid unnecessary allocations, preallocated memory exists to fit all chunks and bonds taking damage once
	// where multiple damage occurs, more memory will be allocated on demand (this may thwart other threads doing the same)
	m_bondBuffer.initialize(m_group->m_bondEventDataBlock.getBlock(m_id), m_group->m_bondEventDataBlock.numElementsPerBlock());
	m_chunkBuffer.initialize(m_group->m_chunkEventDataBlock.getBlock(m_id), m_group->m_chunkEventDataBlock.numElementsPerBlock());

	TkAtomicJobQueue& q = m_group->m_jobQueue;
	TkWorkerJob* j;

	while ((j = q.next()) != nullptr)
	{
		PERF_SCOPE_M("TkActor");

		TkActorImpl* tkActor = j->m_tkActor;
		const uint32_t tkActorIndex = tkActor->getIndex();
		NvBlastActor* actorLL = tkActor->getActorLLInternal();
		TkFamilyImpl& family = tkActor->getFamilyImpl();
		SharedMemory* mem = m_group->getSharedMemory(&family);
		TkEventQueue& events = mem->m_events;

		NVBLAST_ASSERT(tkActor->getGroupImpl() == m_group);

#if NV_PROFILE
		*timers += tkActor->m_timers;
		NvBlastTimersReset(&tkActor->m_timers);
		jobCount++;
#endif

		// generate and apply fracture for all damage requested on this actor
		// and queue events accordingly
		for (const auto& damage : tkActor->m_damageBuffer)
		{
			NvBlastFractureBuffers commandBuffer = tempBuffer;

			PERF_ZONE_BEGIN("Material");
			damage.generateFracture(&commandBuffer, actorLL, timers);
			PERF_ZONE_END("Material");

			if (commandBuffer.chunkFractureCount > 0 || commandBuffer.bondFractureCount > 0)
			{
				PERF_SCOPE_M("Fill Command Events");
				reportFractureCommands(commandBuffer, m_bondBuffer, m_chunkBuffer, events, tkActor);
			}

			NvBlastFractureBuffers eventBuffer = tempBuffer;

			PERF_ZONE_BEGIN("Fracture");
			NvBlastActorApplyFracture(&eventBuffer, actorLL, &commandBuffer, TkFrameworkImpl::get()->log, timers);
			PERF_ZONE_END("Fracture");

			if (eventBuffer.chunkFractureCount > 0 || eventBuffer.bondFractureCount > 0)
			{
				PERF_SCOPE_M("Fill Fracture Events");
				tkActor->m_flags |= (TkActorFlag::DAMAGED);
				reportFractureEvents(eventBuffer, m_bondBuffer, m_chunkBuffer, events, tkActor);
			}
		}


		// split the actor, which could have been damaged directly though the TkActor's fracture functions
		// i.e. it did not have damage queued for the above loop

		NvBlastActorSplitEvent splitEvent = { nullptr, nullptr };
		if (tkActor->isDamaged())
		{
			PERF_ZONE_BEGIN("Split Memory");
			uint32_t maxActorCount = NvBlastActorGetMaxActorCountForSplit(actorLL, TkFrameworkImpl::get()->log); 
			splitEvent.newActors = mem->reserveNewActors(maxActorCount);
			PERF_ZONE_END("Split Memory");
			PERF_ZONE_BEGIN("Split");
			j->m_newActorsCount = NvBlastActorSplit(&splitEvent, actorLL, maxActorCount, splitScratch, TkFrameworkImpl::get()->log, timers);
			PERF_ZONE_END("Split");

			tkActor->m_flags.clear(TkActorFlag::DAMAGED);
		}
		else
		{
			j->m_newActorsCount = 0;
		}


		// update the TkActor according to the LL split results and queue events accordingly
		if (j->m_newActorsCount > 0)
		{
			NVBLAST_ASSERT(splitEvent.deletedActor == tkActor->getActorLL());

			PERF_ZONE_BEGIN("memory new actors");

			auto tkSplitEvent = events.allocData<TkSplitEvent>();

			tkSplitEvent->children = mem->reserveNewTkActors(j->m_newActorsCount);
			tkSplitEvent->numChildren = j->m_newActorsCount;

			tkSplitEvent->parentData.family = &family;
			tkSplitEvent->parentData.userData = tkActor->userData;
			tkSplitEvent->parentData.index = tkActorIndex;
			family.removeActor(tkActor);

			PERF_ZONE_END("memory new actors");


			PERF_ZONE_BEGIN("create new actors");
			for (uint32_t i = 0; i < j->m_newActorsCount; ++i)
			{
				TkActorImpl* newActor = family.addActor(splitEvent.newActors[i]);
				tkSplitEvent->children[i] = newActor;
			}
			j->m_newActors = reinterpret_cast<TkActorImpl**>(tkSplitEvent->children);
			PERF_ZONE_END("create new actors");

			PERF_ZONE_BEGIN("split event");
			events.addEvent(tkSplitEvent);
			PERF_ZONE_END("split event");
		}
	}

#if NV_PROFILE
	PERF_ZONE_BEGIN("write timers");
	m_stats.timers = *timers;
	m_stats.processedActorsCount = jobCount;
	m_stats.workerTime = workTime.getElapsedTicks();
	PERF_ZONE_END("write timers");
#endif
}

void TkWorker::release()
{
	m_group->m_sync.notify();
}
