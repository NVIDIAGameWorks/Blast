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


#include "NvBlastExtSync.h"
#include "NvBlastAssert.h"
#include "NvBlast.h"
#include "NvBlastExtPxManager.h"
#include "NvBlastExtPxFamily.h"
#include "NvBlastExtPxActor.h"
#include "PxRigidDynamic.h"

#include <chrono>
using namespace std::chrono;

namespace Nv
{
namespace Blast
{

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//											ExtSyncImpl Definition
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ExtSyncImpl : public ExtSync
{
	NV_NOCOPY(ExtSyncImpl)

public:
	//////// ctor ////////

	ExtSyncImpl();
	
	virtual				~ExtSyncImpl();


	//////// TkEventListener interface ////////

	virtual void		receive(const TkEvent* events, uint32_t eventCount) override;


	//////// ExtSync interface ////////

	virtual void		release() override;

	virtual void		syncFamily(const TkFamily& family) override;
	virtual void		syncFamily(const ExtPxFamily& family) override;

	virtual uint32_t	getSyncBufferSize() const override;
	virtual void		acquireSyncBuffer(const ExtSyncEvent*const*& buffer, uint32_t& size) const override;
	virtual void		releaseSyncBuffer() override;

	virtual void		applySyncBuffer(TkFramework& framework, const ExtSyncEvent** buffer, uint32_t size, TkGroup* groupForNewActors, ExtPxManager* manager) override;


private:
	//////// data ////////

	std::vector<ExtSyncEvent*>		 m_syncEvents;
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//											ExtSyncEvent Implementation
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ExtSyncEvent::release()
{
	NVBLAST_DELETE(this, ExtSyncEvent);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//											ExtSyncImpl Implementation
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ExtSync* ExtSync::create()
{
	return NVBLAST_NEW(ExtSyncImpl) ();
}

void ExtSyncImpl::release()
{
	NVBLAST_DELETE(this, ExtSyncImpl);
}

ExtSyncImpl::ExtSyncImpl()
{
}

ExtSyncImpl::~ExtSyncImpl()
{
	releaseSyncBuffer();
}

void ExtSyncImpl::receive(const TkEvent* events, uint32_t eventCount)
{
	for (uint32_t i = 0; i < eventCount; ++i)
	{
		const TkEvent& tkEvent = events[i];
		if (tkEvent.type == TkEvent::FractureCommand)
		{
			const TkFractureCommands* fracEvent = tkEvent.getPayload<TkFractureCommands>();
			ExtSyncEventFracture* e = NVBLAST_NEW(ExtSyncEventFracture) ();
			e->timestamp = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
			e->familyID = fracEvent->tkActorData.family->getID();
			e->bondFractures.resize(fracEvent->buffers.bondFractureCount);
			e->chunkFractures.resize(fracEvent->buffers.chunkFractureCount);
			memcpy(e->bondFractures.data(), fracEvent->buffers.bondFractures, e->bondFractures.size() * sizeof(NvBlastBondFractureData));
			memcpy(e->chunkFractures.data(), fracEvent->buffers.chunkFractures, e->chunkFractures.size() * sizeof(NvBlastChunkFractureData));
			m_syncEvents.push_back(e);
		}
	}
}

void ExtSyncImpl::syncFamily(const TkFamily& family)
{
	ExtSyncEventFamilySync* e = NVBLAST_NEW(ExtSyncEventFamilySync) ();
	e->timestamp = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
	e->familyID = family.getID();
	const NvBlastFamily* familyLL = family.getFamilyLL();
	const uint32_t size = NvBlastFamilyGetSize(familyLL, logLL);
	e->family = std::vector<char>((char*)familyLL, (char*)familyLL + size);
	m_syncEvents.push_back(e);
}

void ExtSyncImpl::syncFamily(const ExtPxFamily& family)
{
	const TkFamily& tkFamily = family.getTkFamily();

	syncFamily(tkFamily);

	ExtSyncEventPhysicsSync* e = NVBLAST_NEW(ExtSyncEventPhysicsSync) ();
	e->timestamp = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
	e->familyID = tkFamily.getID();
	std::vector<ExtPxActor*> actors(family.getActorCount());
	family.getActors(actors.data(), static_cast<uint32_t>(actors.size()));
	e->data.reserve(actors.size());
	for (ExtPxActor* actor : actors)
	{
		ExtSyncEventPhysicsSync::ActorData data;
		data.transform = actor->getPhysXActor().getGlobalPose();
		data.actorIndex = actor->getTkActor().getIndex();
		e->data.push_back(data);
	}

	m_syncEvents.push_back(e);
}

uint32_t ExtSyncImpl::getSyncBufferSize() const
{
	return static_cast<uint32_t>(m_syncEvents.size());
}

void ExtSyncImpl::acquireSyncBuffer(const ExtSyncEvent* const*& buffer, uint32_t& size) const
{
	buffer = m_syncEvents.data();
	size = static_cast<uint32_t>(m_syncEvents.size());
}

void ExtSyncImpl::releaseSyncBuffer()
{
	for (uint32_t i = 0; i < m_syncEvents.size(); ++i)
	{
		NVBLAST_DELETE(m_syncEvents[i], ExtSyncEvent);
	}
	m_syncEvents.clear();
}

void ExtSyncImpl::applySyncBuffer(TkFramework& framework, const ExtSyncEvent** buffer, uint32_t size, TkGroup* groupForNewActors, ExtPxManager* manager)
{
	const TkType* familyType = framework.getType(TkTypeIndex::Family);
	NVBLAST_ASSERT(familyType);

	for (uint32_t i = 0; i < size; ++i)
	{
		const ExtSyncEvent* e = buffer[i];
		const NvBlastID& id = e->familyID;
		TkIdentifiable* object = framework.findObjectByID(id);
		if (object && object->getType() == *familyType)
		{
			TkFamily* family = static_cast<TkFamily*>(object);

			if (e->type == ExtSyncEventFracture::EVENT_TYPE)
			{
				const ExtSyncEventFracture* fractureEvent = e->getEvent<ExtSyncEventFracture>();
				const NvBlastFractureBuffers commands =
				{
					static_cast<uint32_t>(fractureEvent->bondFractures.size()),
					static_cast<uint32_t>(fractureEvent->chunkFractures.size()),
					const_cast<NvBlastBondFractureData*>(fractureEvent->bondFractures.data()),
					const_cast<NvBlastChunkFractureData*>(fractureEvent->chunkFractures.data())
				};
				family->applyFracture(&commands);
			}
			else if (e->type == ExtSyncEventFamilySync::EVENT_TYPE)
			{
				const ExtSyncEventFamilySync* familyEvent = e->getEvent<ExtSyncEventFamilySync>();
				family->reinitialize((NvBlastFamily*)familyEvent->family.data(), groupForNewActors);
			}
			else if (e->type == ExtSyncEventPhysicsSync::EVENT_TYPE && manager)
			{
				const ExtSyncEventPhysicsSync* physicsEvent = e->getEvent<ExtSyncEventPhysicsSync>();
				ExtPxFamily* pxFamily = manager->getFamilyFromTkFamily(*family);
				if (pxFamily)
				{
					std::vector<ExtPxActor*> actors(pxFamily->getActorCount());
					pxFamily->getActors(actors.data(), static_cast<uint32_t>(actors.size()));

					for (auto data : physicsEvent->data)
					{
						for (ExtPxActor* physicsaActor : actors)
						{
							if (data.actorIndex == physicsaActor->getTkActor().getIndex())
							{
								physicsaActor->getPhysXActor().setGlobalPose(data.transform);
							}
						}
					}
				}
			}
		}
	}
}

} // namespace Blast
} // namespace Nv
