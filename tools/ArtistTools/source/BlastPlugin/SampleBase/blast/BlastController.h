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
// Copyright (c) 2008-2017 NVIDIA Corporation. All rights reserved.


#ifndef BLAST_CONTROLLER_H
#define BLAST_CONTROLLER_H

#include "SampleManager.h"
#include "BlastFamily.h"
#include "DebugRenderBuffer.h"
#include "PxSimulationEventCallback.h"
#include "NvBlastExtImpactDamageManager.h"

using namespace physx;

class BlastAsset;
class BlastReplay;

namespace physx
{
class PxTaskManager;
}
namespace Nv
{
namespace Blast
{
class TkFramework;
class ExtGroupTaskManager;
class ExtSerialization;
}
}


struct BlastTimers
{
	double blastDamageMaterial;
	double blastDamageFracture;
	double blastSplitIsland;
	double blastSplitPartition;
	double blastSplitVisibility;
};

/**
Blast Controller. Entry point for all blast related code, keeps blast actors and controls them.
*/
class BlastController : public ISampleController
{
public:
	//////// ctor ////////

	BlastController();
	virtual ~BlastController();

	void reinitialize();

	//////// controller callbacks ////////

	virtual void onSampleStart();
	virtual void onSampleStop();

	virtual void Animate(double dt);
	void drawUI();


	//////// public API ////////

	bool overlap(const PxGeometry& geometry, const PxTransform& pose, std::function<void(ExtPxActor*)> hitCall);

	bool stressDamage(ExtPxActor *actor, PxVec3 position, PxVec3 force);

	BlastFamilyPtr spawnFamily(BlastAsset* blastAsset, const BlastAsset::ActorDesc& desc);
	void removeFamily(BlastFamilyPtr actor);
	void removeAllFamilies();


	//////// public getters/setters ////////

	TkFramework& getTkFramework() const
	{
		return *m_tkFramework;
	}

	TkGroup* getTkGroup() const
	{
		return m_tkGroup;
	}

	ExtPxManager& getExtPxManager() const
	{
		return *m_extPxManager;
	}

	ExtImpactDamageManager*	getExtImpactDamageManager() const
	{
		return m_extImpactDamageManager;
	}

	BlastReplay* getReplay() const
	{
		return m_replay;
	}

	uint32_t getActorCount() const;

	uint32_t getTotalVisibleChunkCount() const;

	size_t getFamilySize() const;

	size_t getBlastAssetsSize() const
	{
		return m_blastAssetsSize;
	}

	const BlastTimers& getLastBlastTimers() const
	{
		return m_lastBlastTimers;
	}

	bool getImpactDamageEnabled() const
	{
		return m_impactDamageEnabled;
	}

	void setImpactDamageEnabled(bool enabled, bool forceUpdate = false);

	ExtStressSolverSettings& getStressSolverSettings()
	{
		return m_extStressSolverSettings;
	}

// Add By Lixu Begin
	std::vector<BlastFamilyPtr>& getFamilies()
	{
		return m_families;
	}

	BlastFamily* getFamilyByPxActor(const PxActor& actor);
	std::vector<PxActor*> getActor(BlastAsset* asset, int chunkId);

	void updateActorRenderableTransform(const PxActor& actor, physx::PxTransform& pos, bool local = false);

	bool isActorVisible(const PxActor& actor);

	bool isAssetFractrued(const PxActor& actor);

	// only update unfractured mode mesh
	void updateModelMeshToProjectParam(const PxActor& actor);
// Add By Lixu End

	float getLastStressDelta() const;

	void notifyPhysXControllerRelease();

	ExtSerialization*	getExtSerialization() const
	{
		return m_extSerialization;
	}

	//////// public variables for UI ////////

	BlastFamily::DebugRenderMode debugRenderMode;
	float debugRenderScale;


	//////// Filter shader enum ////////

	enum FilterDataAttributes
	{
		SUPPRESS_CONTACT_NOTIFY = 1,
	};

private:
	//////// impact damage event callback ////////

	class EventCallback : public PxSimulationEventCallback
	{
	public:
		EventCallback(ExtImpactDamageManager* manager) : m_manager(manager) {}

		// implemented
		virtual void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, uint32_t nbPairs)
		{
			m_manager->onContact(pairHeader, pairs, nbPairs);
		}

	private:
		// unused
		void onConstraintBreak(PxConstraintInfo*, PxU32) {}
		void onWake(PxActor**, PxU32) {}
		void onSleep(PxActor**, PxU32) {}
		void onTrigger(PxTriggerPair*, PxU32) {}
		void onAdvance(const PxRigidBody*const*, const PxTransform*, const PxU32) {}

		// data
		ExtImpactDamageManager*		m_manager;
	};


	//////// private methods ////////

	void updateDraggingStress();

	void updateImpactDamage();

	void refreshImpactDamageSettings();

	void fillDebugRender();

	void recalculateAssetsSize();

	static bool customImpactDamageFunction(void* data, ExtPxActor* actor, PxShape* shape, PxVec3 position, PxVec3 force);


	//////// used controllers ////////

	Renderer& getRenderer() const
	{
		return getManager()->getRenderer();
	}

	PhysXController& getPhysXController() const
	{
		return getManager()->getPhysXController();
	}


	//////// internal data ////////

	PxTaskManager*					   m_taskManager;
	TkFramework*					   m_tkFramework;
	TkGroup*						   m_tkGroup;
	ExtPxManager*					   m_extPxManager;
	ExtImpactDamageManager*	           m_extImpactDamageManager;
	ExtImpactSettings				   m_extImpactDamageManagerSettings;
	EventCallback*					   m_eventCallback;
	ExtStressSolverSettings			   m_extStressSolverSettings;
	ExtGroupTaskManager*			   m_extGroupTaskManager;
	ExtSerialization*				   m_extSerialization;

	std::vector<BlastFamilyPtr>		   m_families;
	DebugRenderBuffer                  m_debugRenderBuffer;

	bool                               m_impactDamageEnabled;
	bool                               m_impactDamageUpdatePending;
	bool							   m_impactDamageToStressEnabled;

	float							   m_impactDamageToStressFactor;
	float							   m_draggingToStressFactor;

	bool							   m_rigidBodyLimitEnabled;
	uint32_t						   m_rigidBodyLimit;

	BlastReplay*					   m_replay;

	BlastTimers				           m_lastBlastTimers;

	size_t					           m_blastAssetsSize;
};


#endif
