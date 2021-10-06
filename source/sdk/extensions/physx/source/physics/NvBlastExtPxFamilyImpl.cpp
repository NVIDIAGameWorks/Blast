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


#include "NvBlastExtPxFamilyImpl.h"
#include "NvBlastExtPxActorImpl.h"
#include "NvBlastExtPxAssetImpl.h"
#include "NvBlastExtPxListener.h"
#include "NvBlastExtPxManagerImpl.h"

#include "NvBlastTkFamily.h"
#include "NvBlastTkActor.h"
#include "NvBlastTkJoint.h"

#include "NvBlastAssert.h"

#include "PxRigidDynamic.h"
#include "PxScene.h"

#include <algorithm>


namespace Nv
{
namespace Blast
{


ExtPxFamilyImpl::ExtPxFamilyImpl(ExtPxManagerImpl& manager, TkFamily& tkFamily, ExtPxAsset& pxAsset)
	: m_manager(manager)
	, m_tkFamily(tkFamily)
	, m_pxAsset(pxAsset)
	, m_pxShapeDescTemplate(nullptr)
	, m_pxActorDescTemplate(nullptr)
	, m_material(nullptr)
	, m_isSpawned(false)
{
	m_subchunkShapes.resize(static_cast<uint32_t>(m_pxAsset.getSubchunkCount()));

	userData = nullptr;

	m_manager.registerFamily(*this);
}

ExtPxFamilyImpl::~ExtPxFamilyImpl()
{
	m_manager.unregisterFamily(*this);

	if (m_isSpawned)
	{
		m_tkFamily.removeListener(*this);

		auto& actors = m_actorsBuffer;
		actors.resize(m_actors.size());
		uint32_t i = 0;
		for (auto it = m_actors.getIterator(); !it.done(); ++it)
		{
			actors[i++] = *it;
		}
		destroyActors(actors.begin(), actors.size());
	}

	m_tkFamily.release();
}

void ExtPxFamilyImpl::release()
{
	NVBLAST_DELETE(this, ExtPxFamilyImpl);
}

bool ExtPxFamilyImpl::spawn(const physx::PxTransform& pose, const physx::PxVec3& scale, const ExtPxSpawnSettings& settings)
{
	NVBLAST_CHECK_ERROR(!m_isSpawned, "Family spawn: family already spawned. Was spawn() called twice?", return false);
	NVBLAST_CHECK_ERROR(settings.scene != nullptr, "Family creation: desc.scene is nullptr", return false);
	NVBLAST_CHECK_ERROR(settings.material != nullptr, "Family creation: desc.material is nullptr", return false);

	m_initialTransform = pose;
	m_spawnSettings = settings;

	// get current tkActors (usually it's only 1, but it can be already in split state)
	const uint32_t actorCount = (uint32_t)m_tkFamily.getActorCount();
	m_newActorsBuffer.resize(actorCount);
	m_tkFamily.getActors(m_newActorsBuffer.begin(), actorCount);

	// calc max split count
	uint32_t splitMaxActorCount = 0;
	for (TkActor* actor : m_newActorsBuffer)
	{
		splitMaxActorCount = std::max<uint32_t>(splitMaxActorCount, actor->getSplitMaxActorCount());
	}

	// preallocate memory
	m_newActorsBuffer.resize(splitMaxActorCount);
	m_newActorCreateInfo.resize(splitMaxActorCount);
	m_physXActorsBuffer.resize(splitMaxActorCount);
	m_physXActorsBuffer.resize(splitMaxActorCount);
	m_indicesScratch.reserve(splitMaxActorCount);

	// fill initial actor create info
	for (uint32_t i = 0; i < actorCount; ++i)
	{
		PxActorCreateInfo& pxActorInfo = m_newActorCreateInfo[i];
		pxActorInfo.m_parentAngularVelocity = PxVec3(PxZero);
		pxActorInfo.m_parentLinearVelocity = PxVec3(PxZero);
		pxActorInfo.m_transform = pose;
		pxActorInfo.m_scale = scale;
	}

	// create first actors in family
	createActors(m_newActorsBuffer.begin(), m_newActorCreateInfo.begin(), actorCount);

	// listen family for new actors
	m_tkFamily.addListener(*this);

	m_isSpawned = true;

	return true;
}

bool ExtPxFamilyImpl::despawn()
{
	NVBLAST_CHECK_ERROR(m_spawnSettings.scene != nullptr, "Family despawn: desc.scene is nullptr", return false);

	auto& actors = m_actorsBuffer;
	actors.resize(m_actors.size());
	uint32_t i = 0;
	for (auto it = m_actors.getIterator(); !it.done(); ++it)
	{
		actors[i++] = *it;
	}
	destroyActors(actors.begin(), actors.size());

	return true;
}

void ExtPxFamilyImpl::receive(const TkEvent* events, uint32_t eventCount)
{
	auto& actorsToDelete = m_actorsBuffer;
	actorsToDelete.clear();
	uint32_t totalNewActorsCount = 0;

	for (uint32_t i = 0; i < eventCount; ++i)
	{
		const TkEvent& e = events[i];
		if (e.type == TkEvent::Split)
		{
			const TkSplitEvent* splitEvent = e.getPayload<TkSplitEvent>();

			uint32_t newActorsCount = splitEvent->numChildren;

			ExtPxActorImpl* parentActor = nullptr;
			PxRigidDynamic* parentPxActor = nullptr;
			if (splitEvent->parentData.userData)
			{
				parentActor = reinterpret_cast<ExtPxActorImpl*>(splitEvent->parentData.userData);
				parentPxActor = &parentActor->getPhysXActor();
			}

			for (uint32_t j = totalNewActorsCount; j < totalNewActorsCount + newActorsCount; ++j)
			{
				const PxTransform parentTransform = parentPxActor ? parentPxActor->getGlobalPose() : m_initialTransform;
				m_newActorCreateInfo[j].m_transform = parentTransform;
				m_newActorCreateInfo[j].m_parentCOM = parentTransform.transform(parentPxActor ? parentPxActor->getCMassLocalPose().p : PxVec3(PxZero));
				
				//TODO: Get the current scale of the actor!
				m_newActorCreateInfo[j].m_scale = m_initialScale;

				m_newActorCreateInfo[j].m_parentLinearVelocity = parentPxActor ? parentPxActor->getLinearVelocity() : PxVec3(PxZero);
				m_newActorCreateInfo[j].m_parentAngularVelocity = parentPxActor ? parentPxActor->getAngularVelocity() : PxVec3(PxZero);

				m_newActorsBuffer[j] = splitEvent->children[j - totalNewActorsCount];
			}

			totalNewActorsCount += newActorsCount;

			if (parentActor)
			{
				actorsToDelete.pushBack(parentActor);
			}
		}
	}

	destroyActors(actorsToDelete.begin(), actorsToDelete.size());
	if (totalNewActorsCount > 0)
	{
		uint32_t cappedNewActorsCount = totalNewActorsCount;
		const uint32_t actorCountLimit = m_manager.getActorCountLimit();
		const uint32_t totalActorCount = m_manager.getPxActorCount();
		if (actorCountLimit > 0 && cappedNewActorsCount + totalActorCount > actorCountLimit)
		{
			cappedNewActorsCount = actorCountLimit > totalActorCount ? actorCountLimit - totalActorCount : 0;
		}
		createActors(m_newActorsBuffer.begin(), m_newActorCreateInfo.begin(), cappedNewActorsCount);
		m_culledActors.reserve(m_culledActors.size() + totalNewActorsCount - cappedNewActorsCount);
		for (uint32_t i = cappedNewActorsCount; i < totalNewActorsCount; ++i)
		{
			m_culledActors.pushBack(m_newActorsBuffer[i]);
		}
		totalNewActorsCount = cappedNewActorsCount;	// In case it's used below
	}

	for (uint32_t i = 0; i < eventCount; ++i)
	{
		const TkEvent& e = events[i];
		if (e.type == TkEvent::JointUpdate)
		{
			const TkJointUpdateEvent* jointEvent = e.getPayload<TkJointUpdateEvent>();
			NVBLAST_ASSERT(jointEvent->joint);
			TkJoint& joint = *jointEvent->joint;

			switch (jointEvent->subtype)
			{
			case TkJointUpdateEvent::External:
				m_manager.createJoint(joint);
				break;
			case TkJointUpdateEvent::Changed:
				m_manager.updateJoint(joint);
				break;
			case TkJointUpdateEvent::Unreferenced:
				m_manager.destroyJoint(joint);
				joint.release();
				break;
			}
		}
	}
}

void ExtPxFamilyImpl::createActors(TkActor** tkActors, const PxActorCreateInfo* pxActorInfos, uint32_t count)
{
	auto actorsToAdd = m_physXActorsBuffer.begin();
	for (uint32_t i = 0; i < count; ++i)
	{
		ExtPxActorImpl* actor = NVBLAST_NEW(ExtPxActorImpl)(this, tkActors[i], pxActorInfos[i]);
		m_actors.insert(actor);
		actorsToAdd[i] = &actor->getPhysXActor();
		dispatchActorCreated(*actor);

		// Handle incomplete joints
		auto e = m_manager.m_incompleteJointMultiMap.find(tkActors[i]);
		if (e != nullptr)
		{
			Array<TkJoint*>::type joints = e->second;	// Copying the array
			m_manager.m_incompleteJointMultiMap.erase(tkActors[i]);
			for (uint32_t j = 0; j < joints.size(); ++j)
			{
				m_manager.updateJoint(*joints[j]);
			}
		}
	}
	m_spawnSettings.scene->addActors(actorsToAdd, static_cast<uint32_t>(count));
}

void ExtPxFamilyImpl::destroyActors(ExtPxActor** actors, uint32_t count)
{
	auto pxActorsToRemove = m_physXActorsBuffer.begin();
	for (uint32_t i = 0; i < count; ++i)
	{
		pxActorsToRemove[i] = &actors[i]->getPhysXActor();
	}
	m_spawnSettings.scene->removeActors(pxActorsToRemove, static_cast<uint32_t>(count));

	for (uint32_t i = 0; i < count; ++i)
	{
		ExtPxActorImpl* actor = (ExtPxActorImpl*)actors[i];
		m_actors.erase(actor);
		dispatchActorDestroyed(*actor);
		NVBLAST_DELETE(actor, ExtPxActorImpl);
	}
}

void ExtPxFamilyImpl::dispatchActorCreated(ExtPxActor& actor)
{
	for (ExtPxListener* listener : m_listeners)
		listener->onActorCreated(*this, actor);
	m_manager.dispatchActorCreated(*this, actor);
}

void ExtPxFamilyImpl::dispatchActorDestroyed(ExtPxActor& actor)
{
	for (ExtPxListener* listener : m_listeners)
		listener->onActorDestroyed(*this, actor);
	m_manager.dispatchActorDestroyed(*this, actor);
}

void ExtPxFamilyImpl::postSplitUpdate()
{
	for (auto actor : m_culledActors)
	{
		actor->release();
	}
	m_culledActors.resize(0);
}


} // namespace Blast
} // namespace Nv
