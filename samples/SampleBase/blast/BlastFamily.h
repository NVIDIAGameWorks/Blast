/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef BLAST_FAMILY_H
#define BLAST_FAMILY_H

#include "BlastAsset.h"
#include "NvBlastExtPxListener.h"
#include "NvBlastExtStressSolver.h"
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


/**
BlastFamily class represents 1 spawned BlastAsset, contains and manipulates all physx/blast actors spawned by fracturing it.
Abstract class, internal actor management functions are implementation dependent and so pure virtual.
*/
class BlastFamily
{
public:

	//////// public API ////////

	void blast(PxVec3 worldPos, float damageRadius, float explosiveImpulse, std::function<void(ExtPxActor*)> damageFunction);
	void explode(PxVec3 worldPos, float damageRadius, float explosiveImpulse);

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

	void reloadStressSolver();


	//////// consts ////////

	static const float BOND_HEALTH_MAX;


	//////// settings ////////

	struct Settings
	{
		bool						stressSolverEnabled;
		ExtStressSolverSettings		stressSolverSettings;
		bool						stressDamageEnabled;
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
	ExtPxManager&	m_pxManager;
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
	ExtPxFamily*										 m_pxFamily;
	PxManagerListener					                 m_listener;
	Settings												 m_settings;
	size_t									                 m_familySize;
	uint32_t                                                 m_totalVisibleChunkCount;
	ExtStressSolver*										 m_stressSolver;
	double													 m_stressSolveTime;
	std::set<ExtPxActor*>								 m_actors;
	std::set<const ExtPxActor*>						 m_actorsToUpdateHealth;
};


#endif //BLAST_FAMILY_H