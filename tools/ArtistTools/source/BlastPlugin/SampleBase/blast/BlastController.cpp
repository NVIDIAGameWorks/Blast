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


#include "BlastController.h"
#include "BlastFamily.h"
#include "BlastAsset.h"
#include "BlastReplay.h"
#include "PhysXController.h"
#include "SampleTime.h"
#include "SampleProfiler.h"
#include "Utils.h"
#include "Renderer.h"

#include "NvBlastExtPxTask.h"

#include "NvBlast.h"
#include "NvBlastPxCallbacks.h"
#include "NvBlastExtPxManager.h"
#include "NvBlastExtPxFamily.h"
#include "NvBlastExtPxActor.h"
#include "NvBlastExtSerialization.h"
#include "NvBlastExtTkSerialization.h"
#include "NvBlastExtPxSerialization.h"

#include "NvBlastTkFramework.h"

#include "PsString.h"
#include "PxTaskManager.h"
#include "PxDefaultCpuDispatcher.h"
#include "PxRigidBody.h"
#include "PxScene.h"
#include "PxRigidDynamic.h"
#include "PxDistanceJoint.h"

#include <sstream>
#include <numeric>
#include <cstdlib>

#include "imgui.h"

// Add By Lixu Begin
#include "BlastSceneTree.h"
// Add By Lixu End

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												Joint creation
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static physx::PxJoint* createPxJointCallback(ExtPxActor* actor0, const physx::PxTransform& localFrame0, ExtPxActor* actor1, const physx::PxTransform& localFrame1, physx::PxPhysics& physics, TkJoint& joint)
{
	PxDistanceJoint* pxJoint = PxDistanceJointCreate(physics, actor0 ? &actor0->getPhysXActor() : nullptr, localFrame0, actor1 ? &actor1->getPhysXActor() : nullptr, localFrame1);
	pxJoint->setMaxDistance(1.0f);
	return pxJoint;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												Controller
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BlastController::BlastController() 
: m_eventCallback(nullptr), debugRenderMode(BlastFamily::DEBUG_RENDER_DISABLED), m_impactDamageEnabled(true), 
m_impactDamageToStressEnabled(false), m_rigidBodyLimitEnabled(true), m_rigidBodyLimit(40000), m_blastAssetsSize(0), debugRenderScale(0.01f),
m_taskManager(nullptr), m_extGroupTaskManager(nullptr)
{
	m_impactDamageToStressFactor = 0.01f;
	m_draggingToStressFactor = 100.0f;
}


BlastController::~BlastController()
{
}

void BlastController::reinitialize()
{
	onSampleStop();
	onSampleStart();
}

void BlastController::onSampleStart()
{
	m_tkFramework = NvBlastTkFrameworkCreate();

	m_replay = new BlastReplay();

	m_taskManager = PxTaskManager::createTaskManager(NvBlastGetPxErrorCallback(), getPhysXController().getCPUDispatcher(), 0);

	TkGroupDesc gdesc;
	gdesc.workerCount = m_taskManager->getCpuDispatcher()->getWorkerCount();
	m_tkGroup = m_tkFramework->createGroup(gdesc);

	m_extPxManager = ExtPxManager::create(getPhysXController().getPhysics(), *m_tkFramework, createPxJointCallback);
	m_extPxManager->setActorCountLimit(m_rigidBodyLimitEnabled ? m_rigidBodyLimit : 0);
	m_extImpactDamageManager = ExtImpactDamageManager::create(m_extPxManager, m_extImpactDamageManagerSettings);
	m_eventCallback = new EventCallback(m_extImpactDamageManager);

	m_extGroupTaskManager = ExtGroupTaskManager::create(*m_taskManager);
	m_extGroupTaskManager->setGroup(m_tkGroup);

	setImpactDamageEnabled(m_impactDamageEnabled, true);

	m_extSerialization = NvBlastExtSerializationCreate();
	if (m_extSerialization != nullptr)
	{
		NvBlastExtTkSerializerLoadSet(*m_tkFramework, *m_extSerialization);
		NvBlastExtPxSerializerLoadSet(*m_tkFramework, getPhysXController().getPhysics(), getPhysXController().getCooking(), *m_extSerialization);
	}
}


void BlastController::onSampleStop()
{
	getPhysXController().simualtionSyncEnd();

	removeAllFamilies();

	m_extImpactDamageManager->release();
	m_extPxManager->release();
	SAFE_DELETE(m_eventCallback);

	m_tkGroup->release();

	delete m_replay;

	m_tkFramework->release();

	if (m_extGroupTaskManager != nullptr)
	{
		m_extGroupTaskManager->release();
		m_extGroupTaskManager = nullptr;
	}

	if (m_taskManager != nullptr)
	{
		m_taskManager->release();
	}
}


void BlastController::notifyPhysXControllerRelease()
{
	if (m_extGroupTaskManager != nullptr)
	{
		m_extGroupTaskManager->release();
		m_extGroupTaskManager = nullptr;
	}

	if (m_taskManager != nullptr)
	{
		m_taskManager->release();
		m_taskManager = nullptr;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												Impact damage
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BlastController::updateImpactDamage()
{
	if (m_impactDamageUpdatePending)
	{
		getPhysXController().getPhysXScene().setSimulationEventCallback(m_impactDamageEnabled ? m_eventCallback : nullptr);
		refreshImpactDamageSettings();
		m_impactDamageUpdatePending = false;
	}
}

void BlastController::setImpactDamageEnabled(bool enabled, bool forceUpdate)
{
	if (m_impactDamageEnabled != enabled || forceUpdate)
	{
		m_impactDamageEnabled = enabled;
		m_impactDamageUpdatePending = true;
	}
}

bool BlastController::customImpactDamageFunction(void* data, ExtPxActor* actor, physx::PxShape* shape, physx::PxVec3 position, physx::PxVec3 force)
{
	return reinterpret_cast<BlastController*>(data)->stressDamage(actor, position, force);
}

bool BlastController::stressDamage(ExtPxActor *actor, physx::PxVec3 position, physx::PxVec3 force)
{
	if (actor->getTkActor().getGraphNodeCount() > 1)
	{
		void* userData = actor->getFamily().userData;
		if (userData)
		{
			ExtPxStressSolver* solver = reinterpret_cast<ExtPxStressSolver*>(userData);
			solver->getSolver().addForce(*actor->getTkActor().getActorLL(), position, force * m_impactDamageToStressFactor);
			return true;
		}
	}

	return false;
}

void BlastController::refreshImpactDamageSettings()
{
	m_extImpactDamageManagerSettings.damageFunction = m_impactDamageToStressEnabled ? customImpactDamageFunction : nullptr;
	m_extImpactDamageManagerSettings.damageFunctionData = this;
	m_extImpactDamageManager->setSettings(m_extImpactDamageManagerSettings);
}

// Add By Lixu Begin
BlastFamily* BlastController::getFamilyByPxActor(const PxActor& actor)
{
	for (BlastFamilyPtr family : m_families)
	{
		if (family->find(actor))
		{
			return family;
		}
	}
	return nullptr;
}

std::vector<PxActor*> BlastController::getActor(BlastAsset* asset, int chunkId)
{
	std::vector<PxActor*> actors;
	for (BlastFamilyPtr family : m_families)
	{
		if (&(family->getBlastAsset()) == asset)
		{
			PxActor* actor = nullptr;
			family->getPxActorByChunkIndex(chunkId, &actor);
			if (actor)
				actors.push_back(actor);
		}
	}
	return actors;
}

void BlastController::updateActorRenderableTransform(const PxActor& actor, physx::PxTransform& pos, bool local)
{
	BlastFamily* family = getFamilyByPxActor(actor);
	if (family == nullptr)
		return;

	family->updateActorRenderableTransform(actor, pos, local);
}

bool BlastController::isActorVisible(const PxActor& actor)
{
	BlastFamily* family = getFamilyByPxActor(actor);
	if (family == nullptr)
		return false;

	return family->isActorVisible(actor);
}

bool BlastController::isAssetFractrued(const PxActor& actor)
{
	BlastFamily* family = getFamilyByPxActor(actor);
	if (family == nullptr)
		return false;

	const BlastAsset& asset = family->getBlastAsset();
	if (asset.getChunkIndexesByDepth(1).size() > 0)
	{
		return true;
	}

	return false;
}

void BlastController::updateModelMeshToProjectParam(const PxActor& actor)
{
	BlastFamily* family = getFamilyByPxActor(actor);
	if (family == nullptr)
		return;

	const BlastAsset& asset = family->getBlastAsset();
	SampleManager::ins()->updateModelMeshToProjectParam(const_cast<BlastAsset*>(&asset));
}
// Add By Lixu End

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Stress
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BlastController::updateDraggingStress()
{
	auto physxController = getPhysXController();
	auto actor = physxController.getDraggingActor();
	if (actor)
	{
		ExtPxActor* pxActor = m_extPxManager->getActorFromPhysXActor(*actor);
		if (pxActor && pxActor->getTkActor().getGraphNodeCount() > 1 && pxActor->getPhysXActor().getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC)
		{
			void* userData = pxActor->getFamily().userData;
			if (userData)
			{
				ExtPxStressSolver* solver = reinterpret_cast<ExtPxStressSolver*>(userData);
				PxTransform t(pxActor->getPhysXActor().getGlobalPose().getInverse());
				PxVec3 dragVector = t.rotate(physxController.getDragVector());
				const float factor = dragVector.magnitudeSquared() * m_draggingToStressFactor;
				solver->getSolver().addForce(*pxActor->getTkActor().getActorLL(), physxController.getDragActorHookLocalPoint(), dragVector.getNormalized() * factor);
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Stats
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t BlastController::getActorCount() const
{
	return std::accumulate(m_families.begin(), m_families.end(), (uint32_t)0, [](uint32_t sum, const BlastFamilyPtr& a)
	{
		return sum += a->getActorCount();
	});
}

uint32_t BlastController::getTotalVisibleChunkCount() const
{
	return std::accumulate(m_families.begin(), m_families.end(), (uint32_t)0, [](uint32_t sum, const BlastFamilyPtr& a)
	{
		return sum += a->getTotalVisibleChunkCount();
	});
}

size_t BlastController::getFamilySize() const
{
	return std::accumulate(m_families.begin(), m_families.end(), (size_t)0, [](size_t sum, const BlastFamilyPtr& a)
	{
		return sum += a->getFamilySize();
	});
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Time
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const double Time::s_secondsPerTick = Time::getTickDuration();


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												Controller events
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BlastController::Animate(double dt)
{
	PROFILER_SCOPED_FUNCTION();

	PROFILER_BEGIN("Apply Impact Damage");
	m_extImpactDamageManager->applyDamage();
	PROFILER_END();

	updateDraggingStress();

	fillDebugRender();

	getPhysXController().simualtionSyncEnd();

	updateImpactDamage();

	Time blastTime;
	for (uint32_t i = 0; i < m_families.size(); ++i)
	{
		if (m_families[i])
		{
			m_families[i]->updatePreSplit(dt);
		}
	}

	m_replay->update();

	PROFILER_BEGIN("Tk Group Process/Sync");

#if 1

	m_extGroupTaskManager->process();
	m_extGroupTaskManager->wait();

#else // process group on main thread

	m_tkGroup->process();

#endif

	PROFILER_END();

	getPhysXController().simulationBegin(dt);

	TkGroupStats gstats;
	m_tkGroup->getStats(gstats);

	this->m_lastBlastTimers.blastDamageMaterial = NvBlastTicksToSeconds(gstats.timers.material);
	this->m_lastBlastTimers.blastDamageFracture = NvBlastTicksToSeconds(gstats.timers.fracture);
	this->m_lastBlastTimers.blastSplitIsland = NvBlastTicksToSeconds(gstats.timers.island);
	this->m_lastBlastTimers.blastSplitPartition = NvBlastTicksToSeconds(gstats.timers.partition);
	this->m_lastBlastTimers.blastSplitVisibility = NvBlastTicksToSeconds(gstats.timers.visibility);

	for (uint32_t i = 0; i < m_families.size(); ++i)
	{
		if (m_families[i])
		{
			m_families[i]->updateAfterSplit(dt);
		}
	}

// Add By Lixu Begin
	BlastSceneTree* pBlastSceneTree = BlastSceneTree::ins();
	bool needUpdateUI = false;

	std::map<BlastAsset*, std::vector<BlastFamily*>>& AssetFamiliesMap = getManager()->getAssetFamiliesMap();
	std::map<BlastAsset*, std::vector<BlastFamily*>>::iterator itAsset;
	std::vector<BlastFamily*>::iterator itFamily;
	uint32_t assetIndex = 0;
	for (itAsset = AssetFamiliesMap.begin(); itAsset != AssetFamiliesMap.end(); itAsset++)
	{
		std::vector<BlastFamily*>& BlastFamilies = itAsset->second;
		for (itFamily = BlastFamilies.begin(); itFamily != BlastFamilies.end(); itFamily++)
		{
			BlastFamily* pFamily = *itFamily;

			std::map<uint32_t, bool>& VisibleChangedChunks = pFamily->getVisibleChangedChunks();
			std::map<uint32_t, bool>::iterator itChunk;
			for (itChunk = VisibleChangedChunks.begin(); itChunk != VisibleChangedChunks.end(); itChunk++)
			{
				pBlastSceneTree->updateVisible(assetIndex, itChunk->first, itChunk->second);
				needUpdateUI = true;
			}
			pFamily->clearVisibleChangedChunks();
		}

		assetIndex++;
	}

	if (needUpdateUI)
	{
		//pBlastSceneTree->updateValues(false);
		SampleManager::ins()->m_bNeedRefreshTree = true;
	}
// Add By Lixu End
}


void BlastController::drawUI()
{
	// impact damage
	bool impactEnabled = getImpactDamageEnabled();
	if (ImGui::Checkbox("Impact Damage", &impactEnabled))
	{
		setImpactDamageEnabled(impactEnabled);
	}
	{
		bool refresh = false;
		refresh |= ImGui::Checkbox("Use Shear Damage", &m_extImpactDamageManagerSettings.shearDamage);
		refresh |= ImGui::DragFloat("Impulse Threshold (Min)", &m_extImpactDamageManagerSettings.impulseMinThreshold);
		refresh |= ImGui::DragFloat("Impulse Threshold (Max)", &m_extImpactDamageManagerSettings.impulseMaxThreshold);
		refresh |= ImGui::DragFloat("Damage (Max)", &m_extImpactDamageManagerSettings.damageMax);
		refresh |= ImGui::DragFloat("Damage Radius (Max)", &m_extImpactDamageManagerSettings.damageRadiusMax);
		refresh |= ImGui::DragFloat("Damage Attenuation", &m_extImpactDamageManagerSettings.damageAttenuation, 1.0f, 0.0f, 1.0f);
		refresh |= ImGui::Checkbox("Impact Damage To Stress Solver", &m_impactDamageToStressEnabled);

		if (refresh)
		{
			refreshImpactDamageSettings();
		}
	}

	ImGui::DragFloat("Impact Damage To Stress Factor", &m_impactDamageToStressFactor, 0.001f, 0.0f, 1000.0f, "%.4f");
	ImGui::DragFloat("Dragging To Stress Factor", &m_draggingToStressFactor, 0.1f, 0.0f, 1000.0f, "%.3f");

	ImGui::Checkbox("Limit Rigid Body Count", &m_rigidBodyLimitEnabled);
	if (m_rigidBodyLimitEnabled)
	{
		ImGui::DragInt("Rigid Body Limit", (int*)&m_rigidBodyLimit, 100, 1000, 100000);
	}
	m_extPxManager->setActorCountLimit(m_rigidBodyLimitEnabled ? m_rigidBodyLimit : 0);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													actor management
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BlastFamilyPtr BlastController::spawnFamily(BlastAsset* blastAsset, const BlastAsset::ActorDesc& desc)
{	
	BlastFamilyPtr actor = blastAsset->createFamily(getPhysXController(), *m_extPxManager, desc);
	m_families.push_back(actor);
	recalculateAssetsSize();
	m_replay->addFamily(&actor->getFamily()->getTkFamily());
	return actor;
}

void BlastController::removeFamily(BlastFamilyPtr actor)
{
	m_replay->removeFamily(&actor->getFamily()->getTkFamily());
	m_families.erase(std::remove(m_families.begin(), m_families.end(), actor), m_families.end());
	recalculateAssetsSize();
	getPhysXController().resetDragging();
// Add By Lixu Begin
	delete actor;
// Add By Lixu End
}

void BlastController::removeAllFamilies()
{
	while (!m_families.empty())
	{
		removeFamily(m_families.back());
	}
	m_replay->reset();
}

void BlastController::recalculateAssetsSize()
{
	std::set<const BlastAsset*> uniquedAssets;
	m_blastAssetsSize = 0;
	for (uint32_t i = 0; i < m_families.size(); ++i)
	{
		if (uniquedAssets.find(&m_families[i]->getBlastAsset()) == uniquedAssets.end())
		{
			m_blastAssetsSize += m_families[i]->getBlastAsset().getBlastAssetSize();
			uniquedAssets.insert(&m_families[i]->getBlastAsset());
		}
	}
}

bool BlastController::overlap(const PxGeometry& geometry, const PxTransform& pose, std::function<void(ExtPxActor*)> hitCall)
{
	PROFILER_SCOPED_FUNCTION();

	bool anyHit = false;
	for (uint32_t i = 0; i < m_families.size(); ++i)
	{
		if (m_families[i])
		{
			anyHit |= m_families[i]->overlap(geometry, pose, hitCall);
		}
	}
	return anyHit;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													debug render
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BlastController::fillDebugRender()
{
	PROFILER_SCOPED_FUNCTION();

	m_debugRenderBuffer.clear();

	if (debugRenderMode != BlastFamily::DEBUG_RENDER_DISABLED)
	{
		getPhysXController().getPhysXScene().setVisualizationParameter(PxVisualizationParameter::eSCALE, 1);
		for (uint32_t i = 0; i < m_families.size(); ++i)
		{
			m_families[i]->fillDebugRender(m_debugRenderBuffer, debugRenderMode, debugRenderScale);
		}
	}
	else
	{
		getPhysXController().getPhysXScene().setVisualizationParameter(PxVisualizationParameter::eSCALE, 0);
	}

	getRenderer().queueRenderBuffer(&m_debugRenderBuffer);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
