/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NvBlastExtImpactDamageManager.h"
#include "NvBlastExtPxManager.h"
#include "NvBlastExtPxFamily.h"
#include "NvBlastExtPxActor.h"
#include "NvBlastExtPxListener.h"

#include "NvBlastAssert.h"

#include "NvBlastExtDamageShaders.h"
#include "NvBlastExtArray.h"
#include "NvBlastExtDefs.h"

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
		NVBLASTEXT_DELETE(this, ExtImpactDamageManagerImpl);
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
	ExtArray<PxContactPairPoint>::type	m_pairPointBuffer;
	bool								m_usePxUserData;

	struct ImpactDamageData
	{
		ExtPxActor*	actor;
		PxVec3				force;
		PxVec3				position;
		PxShape*			shape;
	};

	ExtArray<ImpactDamageData>::type	m_impactDamageBuffer;

	NvBlastFractureBuffers				m_fractureBuffers;
	ExtArray<uint8_t>::type				m_fractureData;
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												ExtImpactDamageManagerImpl
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ExtImpactDamageManager* ExtImpactDamageManager::create(ExtPxManager* pxManager, ExtImpactSettings settings)
{
	return NVBLASTEXT_NEW(ExtImpactDamageManagerImpl) (pxManager, settings);
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

float clampedLerp(float from, float to, float t)
{
	t = PxClamp(t, 0.0f, 1.0f);
	return (1 - t) * from + to * t;
}

void ExtImpactDamageManagerImpl::applyDamage()
{
	const auto damageFn = m_settings.damageFunction;
	const auto damageFnData = m_settings.damageFunctionData;

	for (const ImpactDamageData& data : m_impactDamageBuffer)
	{
		float forceMag = data.force.magnitude();
		float acceleration = forceMag / data.actor->getPhysXActor().getMass();
		float factor = acceleration * m_settings.fragility * 0.001f;
		if (factor > 0.05f)
		{
			PxTransform t(data.actor->getPhysXActor().getGlobalPose().getInverse());
			PxVec3 force = t.rotate(data.force);
			PxVec3 position = t.transform(data.position);

			if (!damageFn || !damageFn(damageFnData, data.actor, data.shape, position, force))
			{
				damageActor(data.actor, data.shape, position, force*.00001f);
			}
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

	NvBlastExtShearDamageDesc damage[] = {
		{
			{ force[0], force[1], force[2] },			// shear
			{ position[0], position[1], position[2] }	// position
		}							
	};

	const void* familyMaterial = actor->getTkActor().getFamily().getMaterial();

	// default material params settings
	const NvBlastExtMaterial defaultMaterial = { 3.0f, 0.1f, 0.2f, 1.5f + 1e-5f, 0.95f };

	NvBlastProgramParams programParams;
	programParams.damageDescCount = 1;
	programParams.damageDescBuffer = &damage;
	programParams.material = familyMaterial == nullptr ? &defaultMaterial : familyMaterial;

	NvBlastDamageProgram program = {
		NvBlastExtShearGraphShader,
		NvBlastExtShearSubgraphShader
	};

	NvBlastFractureBuffers fractureEvents = m_fractureBuffers;

	actor->getTkActor().generateFracture(&fractureEvents, program, &programParams);
	actor->getTkActor().applyFracture(nullptr, &fractureEvents);
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
