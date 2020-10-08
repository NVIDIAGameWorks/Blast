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


#include "NvBlastExtImpactDamageManager.h"
#include "NvBlastExtPxManager.h"
#include "NvBlastExtPxFamily.h"
#include "NvBlastExtPxActor.h"
#include "NvBlastExtPxAsset.h"
#include "NvBlastExtPxListener.h"

#include "NvBlastAssert.h"

#include "NvBlastExtDamageShaders.h"
#include "NvBlastArray.h"

#include "PxRigidDynamic.h"
#include "PxSimulationEventCallback.h"
#include "PxRigidBodyExt.h"

#include "NvBlastTkFramework.h"
#include "NvBlastTkActor.h"
#include "NvBlastTkFamily.h"
#include "NvBlastTkAsset.h"


namespace Nv
{
namespace Blast
{

using namespace physx;

const float MIN_IMPACT_VELOCITY_SQUARED = 1.0f;


class ExtImpactDamageManagerImpl final : public ExtImpactDamageManager
{
public:
	ExtImpactDamageManagerImpl(ExtPxManager* pxManager, ExtImpactSettings settings)
		: m_pxManager(pxManager), m_settings(settings), m_listener(this), m_usePxUserData(m_pxManager->isPxUserDataUsed())
	{
		NVBLAST_ASSERT_WITH_MESSAGE(pxManager != nullptr, "ExtImpactDamageManager creation: input ExtPxManager is nullptr.");
		m_pxManager->subscribe(m_listener);

		m_impactDamageBuffer.reserve(32);
	}

	~ExtImpactDamageManagerImpl()
	{
		m_pxManager->unsubscribe(m_listener);
	}

	virtual void					release() override
	{
		NVBLAST_DELETE(this, ExtImpactDamageManagerImpl);
	}


	//////// interface ////////

	virtual void					setSettings(const ExtImpactSettings& settings) override
	{
		m_settings = settings;
	}

	virtual void					onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, uint32_t nbPairs) override;

	virtual void					applyDamage() override;


	//////// public methods ////////

	void							queueImpactDamage(ExtPxActor* actor, PxVec3 force, PxVec3 position, PxShape* shape)
	{
		ImpactDamageData data = { actor, force, position, shape };
		m_impactDamageBuffer.pushBack(data);
	}


private:
	//////// physics manager listener ////////

	class PxManagerListener : public ExtPxListener
	{
	public:
		PxManagerListener(ExtImpactDamageManagerImpl* manager) : m_manager(manager) {}

		virtual void onActorCreated(ExtPxFamily&, ExtPxActor&) override {}
		virtual void onActorDestroyed(ExtPxFamily& family, ExtPxActor& actor) override
		{
			NV_UNUSED(family);

			// filter out actor from queued buffer
			auto& buffer = m_manager->m_impactDamageBuffer;
			for (int32_t i = 0; i < (int32_t)buffer.size(); ++i)
			{
				if (buffer[i].actor == &actor)
				{
					buffer.replaceWithLast(i);
					i--;
				}
			}
		}
	private:
		ExtImpactDamageManagerImpl* m_manager;
	};


	//////// private methods ////////

	void ensureBuffersSize(ExtPxActor* actor);
	void damageActor(ExtPxActor* actor, PxShape* shape, PxVec3 position, PxVec3 force);


	//////// data ////////

	ExtPxManager*						m_pxManager;
	ExtImpactSettings					m_settings;
	PxManagerListener					m_listener;
	Array<PxContactPairPoint>::type		m_pairPointBuffer;
	bool								m_usePxUserData;

	struct ImpactDamageData
	{
		ExtPxActor*	actor;
		PxVec3				force;
		PxVec3				position;
		PxShape*			shape;
	};

	Array<ImpactDamageData>::type		m_impactDamageBuffer;

	NvBlastFractureBuffers				m_fractureBuffers;
	Array<uint8_t>::type				m_fractureData;
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												ExtImpactDamageManagerImpl
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ExtImpactDamageManager* ExtImpactDamageManager::create(ExtPxManager* pxManager, ExtImpactSettings settings)
{
	return NVBLAST_NEW(ExtImpactDamageManagerImpl) (pxManager, settings);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												onContact callback call
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ExtImpactDamageManagerImpl::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, uint32_t nbPairs)
{
	if (pairHeader.flags & physx::PxContactPairHeaderFlag::eREMOVED_ACTOR_0 ||
		pairHeader.flags & physx::PxContactPairHeaderFlag::eREMOVED_ACTOR_1 ||
		pairHeader.actors[0] == nullptr ||
		pairHeader.actors[1] == nullptr)
	{
		return;
	}

	PxRigidActor* rigidActor0 = pairHeader.actors[0];
	PxRigidActor* rigidActor1 = pairHeader.actors[1];

	ExtPxActor* actors[2];

	if (m_usePxUserData)
	{
		actors[0] = (ExtPxActor*)rigidActor0->userData;
		actors[1] = (ExtPxActor*)rigidActor1->userData;
	}
	else
	{
		PxRigidDynamic* rigidDynamic0 = rigidActor0->is<PxRigidDynamic>();
		PxRigidDynamic* rigidDynamic1 = rigidActor1->is<PxRigidDynamic>();
		actors[0] = rigidDynamic0 ? m_pxManager->getActorFromPhysXActor(*rigidDynamic0) : nullptr;
		actors[1] = rigidDynamic1 ? m_pxManager->getActorFromPhysXActor(*rigidDynamic1) : nullptr;
	}
	

	// check one of them is blast actor
	if (actors[0] == nullptr && actors[1] == nullptr)
	{
		return;
	}

	// self-collision check
	if (actors[0] != nullptr && actors[1] != nullptr)
	{
		if (&actors[0]->getFamily() == &actors[1]->getFamily() && !m_settings.isSelfCollissionEnabled)
			return;
	}

	for (uint32_t pairIdx = 0; pairIdx < nbPairs; pairIdx++)
	{
		const PxContactPair& currentPair = pairs[pairIdx];

		if (currentPair.flags & physx::PxContactPairFlag::eREMOVED_SHAPE_0 ||
			currentPair.flags & physx::PxContactPairFlag::eREMOVED_SHAPE_1 ||
			currentPair.shapes[0] == nullptr ||
			currentPair.shapes[1] == nullptr)
		{
			continue;
		}

		float masses[2] = { 0, 0 };
		{
			for (int i = 0; i < 2; ++i)
			{
				PxRigidDynamic* rigidDynamic = pairHeader.actors[i]->is<physx::PxRigidDynamic>();
				if (rigidDynamic)
				{
					if (!(rigidDynamic->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC))
					{
						masses[i] = rigidDynamic->getMass();
					}
				}
			}
		};

		float reducedMass;
		if (masses[0] == 0.0f)
		{
			reducedMass = masses[1];
		}
		else if (masses[1] == 0.0f)
		{
			reducedMass = masses[0];
		}
		else
		{
			reducedMass = masses[0] * masses[1] / (masses[0] + masses[1]);
		}


		PxVec3 destructibleForces[2] = { PxVec3(0.0f), PxVec3(0.0f) };
		PxVec3 avgContactPosition = PxVec3(0.0f);
		PxVec3 avgContactNormal = PxVec3(0.0f);
		uint32_t  numContacts = 0;

		m_pairPointBuffer.resize(currentPair.contactCount);
		uint32_t numContactsInStream = currentPair.contactCount > 0 ? currentPair.extractContacts(m_pairPointBuffer.begin(), currentPair.contactCount) : 0;

		for (uint32_t contactIdx = 0; contactIdx < numContactsInStream; contactIdx++)
		{
			PxContactPairPoint& currentPoint = m_pairPointBuffer[contactIdx];

			const PxVec3& patchNormal = currentPoint.normal;
			const PxVec3& position = currentPoint.position;
			PxVec3 velocities[2] = { PxVec3(0.0f), PxVec3(0.0f) };
			for (int i = 0; i < 2; ++i)
			{
				PxRigidBody* rigidBody = pairHeader.actors[i]->is<physx::PxRigidBody>();
				if (rigidBody)
				{
					velocities[i] = physx::PxRigidBodyExt::getVelocityAtPos(*rigidBody, position);
				}
			}

			const PxVec3 velocityDelta = velocities[0] - velocities[1];
			if (velocityDelta.magnitudeSquared() >= MIN_IMPACT_VELOCITY_SQUARED || reducedMass == 0.0f)	// If reduced mass == 0, this is kineamtic vs. kinematic.  Generate damage.
			{
				for (int i = 0; i < 2; ++i)
				{
					if (actors[i])
					{
						// this is not really physically correct, but at least its deterministic...
						destructibleForces[i] += (patchNormal * patchNormal.dot(velocityDelta)) * reducedMass * (i ? 1.0f : -1.0f);
					}
				}
				avgContactPosition += position;
				avgContactNormal += patchNormal;
				numContacts++;
			}
		}

		if (numContacts)
		{
			avgContactPosition /= (float)numContacts;
			avgContactNormal.normalize();
			for (uint32_t i = 0; i < 2; i++)
			{
				const PxVec3 force = destructibleForces[i] / (float)numContacts;
				ExtPxActor* actor = actors[i];
				if (actor != nullptr)
				{
					if (!force.isZero())
					{
						queueImpactDamage(actor, force, avgContactPosition, currentPair.shapes[i]);
					}
					else if (reducedMass == 0.0f)	// Handle kinematic vs. kinematic
					{
						// holy molly
					}
				}
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//										ExtImpactDamageManager damage processing
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ExtImpactDamageManagerImpl::applyDamage()
{
	const auto damageFn = m_settings.damageFunction;
	const auto damageFnData = m_settings.damageFunctionData;

	for (const ImpactDamageData& data : m_impactDamageBuffer)
	{
		PxTransform t(data.actor->getPhysXActor().getGlobalPose().getInverse());
		PxVec3 force = t.rotate(data.force);
		PxVec3 position = t.transform(data.position);

		if (!damageFn || !damageFn(damageFnData, data.actor, data.shape, position, force))
		{
			damageActor(data.actor, data.shape, position, force);
		}
	}
	m_impactDamageBuffer.clear();
}

void ExtImpactDamageManagerImpl::ensureBuffersSize(ExtPxActor* actor)
{
	const TkAsset* tkAsset = actor->getTkActor().getAsset();
	const uint32_t chunkCount = tkAsset->getChunkCount();
	const uint32_t bondCount = tkAsset->getBondCount();

	m_fractureBuffers.bondFractureCount = bondCount;
	m_fractureBuffers.chunkFractureCount = chunkCount;
	m_fractureData.resize((uint32_t)(m_fractureBuffers.bondFractureCount*sizeof(NvBlastBondFractureData) + m_fractureBuffers.chunkFractureCount*sizeof(NvBlastChunkFractureData))); // chunk count + bond count
	m_fractureBuffers.chunkFractures = reinterpret_cast<NvBlastChunkFractureData*>(m_fractureData.begin());
	m_fractureBuffers.bondFractures = reinterpret_cast<NvBlastBondFractureData*>(&m_fractureData.begin()[m_fractureBuffers.chunkFractureCount*sizeof(NvBlastChunkFractureData)]);
}

void ExtImpactDamageManagerImpl::damageActor(ExtPxActor* actor, PxShape* /*shape*/, PxVec3 position, PxVec3 force)
{
	ensureBuffersSize(actor);

	const float damage = m_settings.hardness > 0.f ? force.magnitude() / m_settings.hardness : 0.f;

	const NvBlastExtMaterial* material = actor->getFamily().getMaterial();
	if (!material)
	{
		return;
	}

	float normalizedDamage = material->getNormalizedDamage(damage);
	if (normalizedDamage == 0.f || normalizedDamage < m_settings.damageThresholdMin)
	{
		return;
	}
	normalizedDamage = PxClamp<float>(normalizedDamage, 0, m_settings.damageThresholdMax);

	const PxVec3 normal = force.getNormalized();
	const float minDistance = m_settings.damageRadiusMax * normalizedDamage;
	const float maxDistance = minDistance * PxClamp<float>(m_settings.damageFalloffRadiusFactor, 1, 32);

	NvBlastExtProgramParams programParams(nullptr);
	programParams.material = material;
	programParams.accelerator = actor->getFamily().getPxAsset().getAccelerator();
	NvBlastDamageProgram program;

	if (m_settings.shearDamage)
	{
		NvBlastExtShearDamageDesc desc = {
			normalizedDamage,
			{ normal[0], normal[1], normal[2] },
			{ position[0], position[1], position[2] },
			minDistance,
			maxDistance
		};

		programParams.damageDesc = &desc;

		program.graphShaderFunction = NvBlastExtShearGraphShader;
		program.subgraphShaderFunction = NvBlastExtShearSubgraphShader;

		NvBlastFractureBuffers fractureEvents = m_fractureBuffers;
		actor->getTkActor().generateFracture(&fractureEvents, program, &programParams);
		actor->getTkActor().applyFracture(nullptr, &fractureEvents);
	}
	else
	{
		NvBlastExtImpactSpreadDamageDesc desc = {
			normalizedDamage,
			{ position[0], position[1], position[2] },
			minDistance,
			maxDistance
		};

		programParams.damageDesc = &desc;

		program.graphShaderFunction = NvBlastExtImpactSpreadGraphShader;
		program.subgraphShaderFunction = NvBlastExtImpactSpreadSubgraphShader;

		NvBlastFractureBuffers fractureEvents = m_fractureBuffers;
		actor->getTkActor().generateFracture(&fractureEvents, program, &programParams);
		actor->getTkActor().applyFracture(nullptr, &fractureEvents);
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//														Filter Shader
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PxFilterFlags ExtImpactDamageManager::FilterShader(
	PxFilterObjectAttributes attributes0,
	PxFilterData filterData0,
	PxFilterObjectAttributes attributes1,
	PxFilterData filterData1,
	PxPairFlags& pairFlags,
	const void* constantBlock,
	uint32_t constantBlockSize)
{
	PX_UNUSED(constantBlock);
	PX_UNUSED(constantBlockSize);
	// let triggers through
	if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
	{
		pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
		return PxFilterFlags();
	}

	if ((PxFilterObjectIsKinematic(attributes0) || PxFilterObjectIsKinematic(attributes1)) &&
		(PxGetFilterObjectType(attributes0) == PxFilterObjectType::eRIGID_STATIC || PxGetFilterObjectType(attributes1) == PxFilterObjectType::eRIGID_STATIC))
	{
		return PxFilterFlag::eSUPPRESS;
	}

	// use a group-based mechanism if the first two filter data words are not 0
	uint32_t f0 = filterData0.word0 | filterData0.word1;
	uint32_t f1 = filterData1.word0 | filterData1.word1;
	if (f0 && f1 && !(filterData0.word0&filterData1.word1 || filterData1.word0&filterData0.word1))
		return PxFilterFlag::eSUPPRESS;

	// determine if we should suppress notification
	const bool suppressNotify = ((filterData0.word3 | filterData1.word3) & ExtPxManager::LEAF_CHUNK) != 0;

	pairFlags = PxPairFlag::eCONTACT_DEFAULT;
	if (!suppressNotify)
	{
		pairFlags = pairFlags
			| PxPairFlag::eNOTIFY_CONTACT_POINTS
			| PxPairFlag::eNOTIFY_THRESHOLD_FORCE_PERSISTS
			| PxPairFlag::eNOTIFY_THRESHOLD_FORCE_FOUND
			| PxPairFlag::eNOTIFY_TOUCH_FOUND
			| PxPairFlag::eNOTIFY_TOUCH_PERSISTS;
	}

	// eSOLVE_CONTACT is invalid with kinematic pairs
	if (PxFilterObjectIsKinematic(attributes0) && PxFilterObjectIsKinematic(attributes1))
	{
		pairFlags &= ~PxPairFlag::eSOLVE_CONTACT;
	}

	return PxFilterFlags();
}

} // namespace Blast
} // namespace Nv
