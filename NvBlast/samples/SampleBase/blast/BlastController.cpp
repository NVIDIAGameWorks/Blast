/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "BlastController.h"
#include "BlastFamily.h"
#include "BlastAsset.h"
#include "BlastReplay.h"
#include "PhysXController.h"
#include "SampleTime.h"
#include "SampleProfiler.h"
#include "Utils.h"
#include "Renderer.h"

#include "NvBlast.h"
#include "NvBlastExtPxManager.h"
#include "NvBlastExtPxFamily.h"
#include "NvBlastExtPxActor.h"

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



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//											   AllocatorCallback
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class BlastAllocatorCallback : public PxAllocatorCallback
{
public:
	virtual void* allocate(size_t size, const char* typeName, const char* filename, int line) override
	{
		NV_UNUSED(typeName);
		NV_UNUSED(filename);
		NV_UNUSED(line);
		return malloc(size);
	}

	virtual void deallocate(void* ptr) override
	{
		free(ptr);
	}
};
BlastAllocatorCallback g_allocatorCallback;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												ErrorCallback
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class BlastErrorCallback : public PxErrorCallback
{
public:
	virtual void reportError(PxErrorCode::Enum code, const char* msg, const char* file, int line) override
	{
		std::stringstream str;
		str << "NvBlastTk ";
		bool critical = false;
		switch (code)
		{
		case PxErrorCode::eNO_ERROR:											critical = false; break;
		case PxErrorCode::eDEBUG_INFO:			str << "[Debug Info]";			critical = false; break;
		case PxErrorCode::eDEBUG_WARNING:		str << "[Debug Warning]";		critical = false; break;
		case PxErrorCode::eINVALID_PARAMETER:	str << "[Invalid Parameter]";	critical = true;  break;
		case PxErrorCode::eINVALID_OPERATION:	str << "[Invalid Operation]";	critical = true;  break;
		case PxErrorCode::eOUT_OF_MEMORY:		str << "[Out of] Memory";		critical = true;  break;
		case PxErrorCode::eINTERNAL_ERROR:		str << "[Internal Error]";		critical = true;  break;
		case PxErrorCode::eABORT:				str << "[Abort]";				critical = true;  break;
		case PxErrorCode::ePERF_WARNING:		str << "[Perf Warning]";		critical = false; break;
		default:								PX_ALWAYS_ASSERT();
		}
		str << file << "(" << line << "): " << msg << "\n";

		std::string message = str.str();
		shdfnd::printString(message.c_str());
		PX_ASSERT_WITH_MESSAGE(!critical, message.c_str());
	}
};
BlastErrorCallback g_errorCallback;


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
m_impactDamageToStressEnabled(false), m_rigidBodyLimitEnabled(true), m_rigidBodyLimit(40000), m_blastAssetsSize(0), debugRenderScale(0.01f)
{
	m_extImpactDamageManagerSettings.fragility = 500.0f;

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
	TkFrameworkDesc desc;
	desc.allocatorCallback = &g_allocatorCallback;
	desc.errorCallback = &g_errorCallback;
	m_tkFramework = NvBlastTkFrameworkCreate(desc);

	m_replay = new BlastReplay();

	m_taskManager = PxTaskManager::createTaskManager(g_errorCallback, getPhysXController().getCPUDispatcher(), 0);

	TkGroupDesc gdesc;
	gdesc.pxTaskManager = m_taskManager;
	m_tkGroup = m_tkFramework->createGroup(gdesc);

	m_extPxManager = ExtPxManager::create(getPhysXController().getPhysics(), *m_tkFramework, createPxJointCallback);
	m_extPxManager->setActorCountLimit(m_rigidBodyLimitEnabled ? m_rigidBodyLimit : 0);
	m_extImpactDamageManager = ExtImpactDamageManager::create(m_extPxManager, m_extImpactDamageManagerSettings);
	m_eventCallback = new EventCallback(m_extImpactDamageManager);

	setImpactDamageEnabled(m_impactDamageEnabled, true);
}


void BlastController::onSampleStop()
{
	removeAllFamilies();

	m_extImpactDamageManager->release();
	m_extPxManager->release();
	SAFE_DELETE(m_eventCallback);

	m_tkGroup->release();

	delete m_replay;

	m_tkFramework->release();

	m_taskManager->release();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												Impact damage
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BlastController::setImpactDamageEnabled(bool enabled, bool forceUpdate)
{
	if (m_impactDamageEnabled != enabled || forceUpdate)
	{
		m_impactDamageEnabled = enabled;
		getPhysXController().getPhysXScene().setSimulationEventCallback(m_impactDamageEnabled ? m_eventCallback : nullptr);
		refreshImpactDamageSettings();
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
			ExtStressSolver* solver = reinterpret_cast<ExtStressSolver*>(userData);
			solver->applyImpulse(*actor, position, force * m_impactDamageToStressFactor);
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
				ExtStressSolver* solver = reinterpret_cast<ExtStressSolver*>(userData);
				PxTransform t(pxActor->getPhysXActor().getGlobalPose().getInverse());
				PxVec3 dragVector = t.rotate(physxController.getDragVector());
				const float factor = dragVector.magnitudeSquared() * m_draggingToStressFactor;
				solver->applyImpulse(*pxActor, physxController.getDragActorHookLocalPoint(), dragVector.getNormalized() * factor);
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

	m_replay->update();

	Time blastTime;
	for (uint32_t i = 0; i < m_families.size(); ++i)
	{
		if (m_families[i])
		{
			m_families[i]->updatePreSplit(dt);
		}
	}

	fillDebugRender();

	PROFILER_BEGIN("Tk Group Process/Sync");
	m_tkGroup->process();
	m_tkGroup->sync(true);
	PROFILER_END();

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
}


void BlastController::drawUI()
{
	// impact damage
	bool impactEnabled = getImpactDamageEnabled();
	if (ImGui::Checkbox("Impact Damage", &impactEnabled))
	{
		setImpactDamageEnabled(impactEnabled);
	}

	if (ImGui::DragFloat("Fragility", &m_extImpactDamageManagerSettings.fragility))
	{
		refreshImpactDamageSettings();
	}

	if (ImGui::Checkbox("Impact Damage To Stress Solver", &m_impactDamageToStressEnabled))
	{
		refreshImpactDamageSettings();
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

void BlastController::blast(PxVec3 worldPos, float damageRadius, float explosiveImpulse, std::function<void(ExtPxActor*)> damageFunction)
{
	PROFILER_SCOPED_FUNCTION();

	for (uint32_t i = 0; i < m_families.size(); ++i)
	{
		if (m_families[i])
		{
			m_families[i]->blast(worldPos, damageRadius, explosiveImpulse, damageFunction);
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													context/log
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BlastController::blastLog(int type, const char* msg, const char* file, int line)
{
	std::stringstream str;
	bool critical = false;
	switch (type)
	{
		case NvBlastMessage::Error:		str << "[NvBlast ERROR] ";   critical = true;  break;
		case NvBlastMessage::Warning:	str << "[NvBlast WARNING] "; critical = true;  break;
		case NvBlastMessage::Info:		str << "[NvBlast INFO] ";    critical = false; break;
		case NvBlastMessage::Debug:		str << "[NvBlast DEBUG] ";   critical = false; break;
	}
	str << file << "(" << line << "): " << msg << "\n";

	std::string message = str.str();
	shdfnd::printString(message.c_str());
	PX_ASSERT_WITH_MESSAGE(!critical, message.c_str());
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
