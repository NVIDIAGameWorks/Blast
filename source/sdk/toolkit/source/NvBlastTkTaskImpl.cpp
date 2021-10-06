// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2016-2020 NVIDIA Corporation. All rights reserved.


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
	uint32_t leafChunkCount = NvBlastAssetGetLeafChunkCount(assetLL, logLL);
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


void TkWorker::initialize()
{
	// temporary memory used to generate and apply fractures
	// it must fit for the largest family involved in the group that owns this worker 
	NvBlastBondFractureData* bondFractureData = m_group->m_bondTempDataBlock.getBlock(m_id);
	uint32_t bondFractureCount = m_group->m_bondTempDataBlock.numElementsPerBlock();
	NvBlastChunkFractureData* chunkFractureData = m_group->m_chunkTempDataBlock.getBlock(m_id);
	uint32_t chunkFractureCount = m_group->m_chunkTempDataBlock.numElementsPerBlock();
	m_tempBuffer = { bondFractureCount, chunkFractureCount, bondFractureData, chunkFractureData };

	// temporary memory used to split the actor
	// large enough for the largest family involved
	m_splitScratch = m_group->m_splitScratchBlock.getBlock(m_id);

	// to avoid unnecessary allocations, preallocated memory exists to fit all chunks and bonds taking damage once
	// where multiple damage occurs, more memory will be allocated on demand (this may thwart other threads doing the same)
	m_bondBuffer.initialize(m_group->m_bondEventDataBlock.getBlock(m_id), m_group->m_bondEventDataBlock.numElementsPerBlock());
	m_chunkBuffer.initialize(m_group->m_chunkEventDataBlock.getBlock(m_id), m_group->m_chunkEventDataBlock.numElementsPerBlock());

#if NV_PROFILE
	NvBlastTimersReset(&m_stats.timers);
	m_stats.processedActorsCount = 0;
#endif
}

void TkWorker::process(TkWorkerJob& j)
{
	NvBlastTimers* timers = nullptr;

	BLAST_PROFILE_SCOPE_M("TkActor");

	TkActorImpl* tkActor = j.m_tkActor;
	const uint32_t tkActorIndex = tkActor->getIndex();
	NvBlastActor* actorLL = tkActor->getActorLLInternal();
	TkFamilyImpl& family = tkActor->getFamilyImpl();
	SharedMemory* mem = m_group->getSharedMemory(&family);
	TkEventQueue& events = mem->m_events;

	NVBLAST_ASSERT(tkActor->getGroupImpl() == m_group);
	NVBLAST_ASSERT(tkActor->m_flags.isSet(TkActorFlag::PENDING));

#if NV_PROFILE
	timers = &m_stats.timers;
	*timers += tkActor->m_timers;
	NvBlastTimersReset(&tkActor->m_timers);
	m_stats.processedActorsCount++;
#endif

	// generate and apply fracture for all damage requested on this actor
	// and queue events accordingly
	for (const auto& damage : tkActor->m_damageBuffer)
	{
		NvBlastFractureBuffers commandBuffer = m_tempBuffer;

		BLAST_PROFILE_ZONE_BEGIN("Material");
		NvBlastActorGenerateFracture(&commandBuffer, actorLL, damage.program, damage.programParams, logLL, timers);
		BLAST_PROFILE_ZONE_END("Material");

		if (commandBuffer.chunkFractureCount > 0 || commandBuffer.bondFractureCount > 0)
		{
			BLAST_PROFILE_SCOPE_M("Fill Command Events");
			reportFractureCommands(commandBuffer, m_bondBuffer, m_chunkBuffer, events, tkActor);
		}

		NvBlastFractureBuffers eventBuffer = m_tempBuffer;

		BLAST_PROFILE_ZONE_BEGIN("Fracture");
		NvBlastActorApplyFracture(&eventBuffer, actorLL, &commandBuffer, logLL, timers);
		BLAST_PROFILE_ZONE_END("Fracture");

		if (eventBuffer.chunkFractureCount > 0 || eventBuffer.bondFractureCount > 0)
		{
			BLAST_PROFILE_SCOPE_M("Fill Fracture Events");
			tkActor->m_flags |= (TkActorFlag::DAMAGED);
			reportFractureEvents(eventBuffer, m_bondBuffer, m_chunkBuffer, events, tkActor);
		}
	}


	// split the actor, which could have been damaged directly though the TkActor's fracture functions
	// i.e. it did not have damage queued for the above loop

	NvBlastActorSplitEvent splitEvent = { nullptr, nullptr };
	if (tkActor->isDamaged())
	{
		BLAST_PROFILE_ZONE_BEGIN("Split Memory");
		uint32_t maxActorCount = NvBlastActorGetMaxActorCountForSplit(actorLL, logLL);
		splitEvent.newActors = mem->reserveNewActors(maxActorCount);
		BLAST_PROFILE_ZONE_END("Split Memory");
		BLAST_PROFILE_ZONE_BEGIN("Split");
		j.m_newActorsCount = NvBlastActorSplit(&splitEvent, actorLL, maxActorCount, m_splitScratch, logLL, timers);
		BLAST_PROFILE_ZONE_END("Split");

		tkActor->m_flags.clear(TkActorFlag::DAMAGED);
	}
	else
	{
		j.m_newActorsCount = 0;
	}


	// update the TkActor according to the LL split results and queue events accordingly
	if (j.m_newActorsCount > 0)
	{
		NVBLAST_ASSERT(splitEvent.deletedActor == tkActor->getActorLL());

		BLAST_PROFILE_ZONE_BEGIN("memory new actors");

		auto tkSplitEvent = events.allocData<TkSplitEvent>();

		tkSplitEvent->children = mem->reserveNewTkActors(j.m_newActorsCount);
		tkSplitEvent->numChildren = j.m_newActorsCount;

		tkSplitEvent->parentData.family = &family;
		tkSplitEvent->parentData.userData = tkActor->userData;
		tkSplitEvent->parentData.index = tkActorIndex;
		family.removeActor(tkActor);

		BLAST_PROFILE_ZONE_END("memory new actors");


		BLAST_PROFILE_ZONE_BEGIN("create new actors");
		for (uint32_t i = 0; i < j.m_newActorsCount; ++i)
		{
			TkActorImpl* newActor = family.addActor(splitEvent.newActors[i]);
			tkSplitEvent->children[i] = newActor;
		}
		j.m_newActors = reinterpret_cast<TkActorImpl**>(tkSplitEvent->children);
		BLAST_PROFILE_ZONE_END("create new actors");

		BLAST_PROFILE_ZONE_BEGIN("split event");
		events.addEvent(tkSplitEvent);
		BLAST_PROFILE_ZONE_END("split event");
	}

	j.m_tkActor->m_flags.clear(TkActorFlag::PENDING);
}


void TkWorker::process(uint32_t jobID)
{
	TkWorkerJob& j = m_group->m_jobs[jobID];
	process(j);
}
