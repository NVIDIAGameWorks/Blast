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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.


#ifndef BLAST_FAMILY_H
#define BLAST_FAMILY_H

#include "BlastAsset.h"
#include "NvBlastExtPxListener.h"
#include "NvBlastExtPxStressSolver.h"
#include "NvBlastExtDamageShaders.h"
#include <functional>
#include <set>


class DebugRenderBuffer;

namespace Nv
{
namespace Blast
{
class TkFamily;
class ExtPxManager;
}
}

namespace physx
{
class PxGeometry;
class PxTransform;
}



/**
BlastFamily class represents 1 spawned BlastAsset, contains and manipulates all physx/blast actors spawned by fracturing it.
Abstract class, internal actor management functions are implementation dependent and so pure virtual.
*/
class BlastFamily
{
public:

	//////// public API ////////

	bool overlap(const PxGeometry& geometry, const PxTransform& pose, std::function<void(ExtPxActor*, BlastFamily&)> hitCall);

	void updatePreSplit(float dt);
	void updateAfterSplit(float dt);

	void drawUI();
	void drawStatsUI();


	enum DebugRenderMode
	{
		DEBUG_RENDER_DISABLED,
		DEBUG_RENDER_HEALTH_GRAPH,
		DEBUG_RENDER_CENTROIDS,
		DEBUG_RENDER_HEALTH_GRAPH_CENTROIDS,
		DEBUG_RENDER_JOINTS,
		DEBUG_RENDER_AABB_TREE_CENTROIDS,
		DEBUG_RENDER_AABB_TREE_SEGMENTS,
		DEBUG_RENDER_STRESS_GRAPH,
		DEBUG_RENDER_STRESS_GRAPH_NODES_IMPULSES,
		DEBUG_RENDER_STRESS_GRAPH_BONDS_IMPULSES,

		// count
		DEBUG_RENDER_MODES_COUNT
	};

	void fillDebugRender(DebugRenderBuffer& debugRenderBuffer, DebugRenderMode mode, float renderScale);


	//////// public getters ////////

	const ExtPxFamily* getFamily() const
	{
		return m_pxFamily;
	}

	const NvBlastExtMaterial& getMaterial() const
	{
		return m_settings.material;
	}

	uint32_t getActorCount() const;

	uint32_t getTotalVisibleChunkCount() const
	{
		return m_totalVisibleChunkCount;
	}

	size_t getFamilySize() const
	{
		return m_familySize;
	}

	const BlastAsset& getBlastAsset()
	{
		return m_blastAsset;
	}

	void resetStress();

	void refreshStressSolverSettings();
	void refreshDamageAcceleratorSettings();

	void reloadStressSolver();


	//////// settings ////////

	struct Settings
	{
		bool						stressSolverEnabled;
		ExtStressSolverSettings		stressSolverSettings;
		bool						stressDamageEnabled;
		bool						damageAcceleratorEnabled;
		NvBlastExtMaterial			material;
	};

	void setSettings(const Settings& settings);

	const Settings& getSettings() const
	{
		return m_settings;
	}


	//////// dtor ////////

	virtual ~BlastFamily();

protected:

	//////// ctor ////////

	BlastFamily(PhysXController& physXController, ExtPxManager& pxManager, const BlastAsset& blastAsset);

	void initialize(const BlastAsset::ActorDesc& desc);


	//////// internal virtual callbacks  ////////

	virtual void onActorCreated(const ExtPxActor& actor) = 0;
	virtual void onActorUpdate(const ExtPxActor& actor) = 0;
	virtual void onActorDestroyed(const ExtPxActor& actor) = 0;
	virtual void onActorHealthUpdate(const ExtPxActor& pxActor) {};
	
	virtual void onUpdate() {}


	//////// protected data ////////

	PhysXController&	m_physXController;
	ExtPxManager&		m_pxManager;
	const BlastAsset&	m_blastAsset;

private:

	//////// physics listener ////////

	class PxManagerListener : public ExtPxListener
	{
	public:
		PxManagerListener(BlastFamily* family) : m_family(family) {}

		virtual void onActorCreated(ExtPxFamily& family, ExtPxActor& actor)
		{
			m_family->processActorCreated(family, actor);

		}

		virtual void onActorDestroyed(ExtPxFamily& family, ExtPxActor& actor)
		{
			m_family->processActorDestroyed(family, actor);
		}
	private:
		BlastFamily* m_family;
	};

	friend class PxManagerListener;

	//////// private methods ////////

	void processActorCreated(ExtPxFamily&, ExtPxActor& actor);
	void processActorDestroyed(ExtPxFamily&, ExtPxActor& actor);


	//////// private data ////////

	TkFamily*												 m_tkFamily;
	ExtPxFamily*										     m_pxFamily;
	PxManagerListener					                     m_listener;
	Settings												 m_settings;
	PxTransform												 m_initialTransform;
	bool													 m_spawned;
	size_t									                 m_familySize;
	uint32_t                                                 m_totalVisibleChunkCount;
	ExtPxStressSolver*										 m_stressSolver;
	double													 m_stressSolveTime;
	std::set<ExtPxActor*>								     m_actors;
	std::set<const ExtPxActor*>						         m_actorsToUpdateHealth;
	int														 m_debugRenderDepth;
};


#endif //BLAST_FAMILY_H