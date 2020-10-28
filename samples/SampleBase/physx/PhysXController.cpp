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
// Copyright (c) 2008-2020 NVIDIA Corporation. All rights reserved.


#include "PhysXController.h"
#include "RenderMaterial.h"
#include "ResourceManager.h"
#include "Renderer.h"

#include "XInput.h"
#include "DXUTMisc.h"
#include "DXUTCamera.h"
#include "ConvexRenderMesh.h"
#include "RenderUtils.h"
#include "SampleProfiler.h"
#include "NvBlastExtCustomProfiler.h"
#include "NvBlastPxCallbacks.h"

#include "PxPhysicsVersion.h"
#include "PxPvdTransport.h"
#include "PxDefaultCpuDispatcher.h"
#include "PxPhysics.h"
#include "PxScene.h"
#include "PxCooking.h"
#include "PxGpu.h"
#include "PxSimpleFactory.h"
#include "PxRigidBodyExt.h"
#include "PxRigidDynamic.h"
#include "PxRigidStatic.h"
#include "PxMaterial.h"
#include "PxMath.h"

#include <imgui.h>
#include <chrono>

using namespace std::chrono;

#define PVD_TO_FILE 0

const DirectX::XMFLOAT4 PLANE_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
const DirectX::XMFLOAT4 HOOK_LINE_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
const float DEFAULT_FIXED_TIMESTEP = 1.0f / 60.0f;


PhysXController::PhysXController(PxSimulationFilterShader filterShader)
: m_filterShader(filterShader)
, m_gpuPhysicsAvailable(true)
, m_isSimulating(false)
, m_useGPUPhysics(true)
, m_lastSimulationTime(0)
, m_paused(false)
, m_draggingActor(nullptr)
, m_draggingEnabled(true)
, m_draggingTryReconnect(false)
, m_perfWriter(NULL)
, m_fixedTimeStep(DEFAULT_FIXED_TIMESTEP)
, m_timeAccumulator(0)
, m_useFixedTimeStep(true)
, m_maxSubstepCount(1)
{
	QueryPerformanceFrequency(&m_performanceFreq);
}

PhysXController::~PhysXController()
{
}

void PhysXController::onInitialize()
{
	initPhysX();
	initPhysXPrimitives();
}

void PhysXController::onTerminate()
{
	simualtionSyncEnd();
	releasePhysXPrimitives();
	releasePhysX();
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												PhysX init/release
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void PhysXController::initPhysX()
{
	m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, NvBlastGetPxAllocatorCallback(), NvBlastGetPxErrorCallback());

	m_pvd = PxCreatePvd(*m_foundation);

	static Nv::Blast::ExtCustomProfiler gBlastProfiler;
	NvBlastProfilerSetCallback(&gBlastProfiler);
	NvBlastProfilerSetDetail(Nv::Blast::ProfilerDetail::LOW);
	gBlastProfiler.setPlatformEnabled(false);

	PxTolerancesScale scale;

	m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, scale, true, m_pvd);

	PxCookingParams cookingParams(scale);
	cookingParams.buildGPUData = true;
	m_cooking = PxCreateCooking(PX_PHYSICS_VERSION, m_physics->getFoundation(), cookingParams);

	PxCudaContextManagerDesc ctxMgrDesc;
	m_cudaContext = PxCreateCudaContextManager(m_physics->getFoundation(), ctxMgrDesc);
	if (m_cudaContext && !m_cudaContext->contextIsValid())
	{
		m_cudaContext->release();
		m_cudaContext = NULL;
	}

	PxSceneDesc sceneDesc(m_physics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	m_dispatcher = PxDefaultCpuDispatcherCreate(4);
	sceneDesc.cpuDispatcher = m_dispatcher;
	sceneDesc.cudaContextManager = m_cudaContext;
	sceneDesc.filterShader = m_filterShader;
	sceneDesc.flags |= PxSceneFlag::eENABLE_STABILIZATION;
	sceneDesc.flags |= PxSceneFlag::eENABLE_PCM;
	if (sceneDesc.cudaContextManager == nullptr)
	{
		m_gpuPhysicsAvailable = false;
		m_useGPUPhysics = false;
	}
	if (m_useGPUPhysics)
	{
		sceneDesc.flags |= PxSceneFlag::eENABLE_GPU_DYNAMICS;
		sceneDesc.broadPhaseType = PxBroadPhaseType::eGPU;

		sceneDesc.gpuDynamicsConfig.constraintBufferCapacity *= 4;
		sceneDesc.gpuDynamicsConfig.contactBufferCapacity *= 4;
		sceneDesc.gpuDynamicsConfig.contactStreamSize *= 4;
		sceneDesc.gpuDynamicsConfig.forceStreamCapacity *= 4;
		sceneDesc.gpuDynamicsConfig.foundLostPairsCapacity *= 4;
		sceneDesc.gpuDynamicsConfig.patchStreamSize *= 4;
		sceneDesc.gpuDynamicsConfig.tempBufferCapacity *= 4;

	}
	m_physicsScene = m_physics->createScene(sceneDesc);

	m_defaultMaterial = m_physics->createMaterial(0.8f, 0.7f, 0.1f);

	PxPvdSceneClient* pvdClient = m_physicsScene->getScenePvdClient();
	if(pvdClient)
	{
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}

	m_physicsScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 0);

#if NV_DEBUG || NV_CHECKED || NV_PROFILE
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("localhost", 5425, 10);
	if (transport)
	{
		m_pvd->connect(*transport,
#if NV_DEBUG || NV_CHECKED
			PxPvdInstrumentationFlag::eALL
#else
			PxPvdInstrumentationFlag::ePROFILE
#endif
			);
	}
#endif
}

void PhysXController::releasePhysX()
{
	m_defaultMaterial->release();
	m_physicsScene->release();
	if (m_cudaContext)
		m_cudaContext->release();
	m_dispatcher->release();
	m_physics->release();
	if (m_pvd)
	{
		PxPvdTransport* transport = m_pvd->getTransport();
		m_pvd->release();
		if (transport)
			transport->release();
	}
	m_cooking->release();
	m_foundation->release();
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												GPU toggle
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void PhysXController::setUseGPUPhysics(bool useGPUPhysics)
{
	if (!m_gpuPhysicsAvailable)
	{
		useGPUPhysics = false;
	}

	if (m_useGPUPhysics == useGPUPhysics)
	{
		return;
	}

	onTerminate();

	m_useGPUPhysics = useGPUPhysics;

	onInitialize();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												PhysX wrappers
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PxRigidDynamic* PhysXController::createRigidDynamic(const PxTransform& transform)
{
	return m_physics->createRigidDynamic(transform);
}

void PhysXController::releaseRigidDynamic(PxRigidDynamic* rigidDynamic)
{
	notifyRigidDynamicDestroyed(rigidDynamic);

	m_physXActorsToRemove.push_back(rigidDynamic);
}

void PhysXController::notifyRigidDynamicDestroyed(PxRigidDynamic* rigidDynamic)
{
	if (m_draggingActor == rigidDynamic)
	{
		m_draggingActor = nullptr;
		m_draggingTryReconnect = true;
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												Simulation control
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void PhysXController::simulationBegin(float dt)
{
	PROFILER_SCOPED_FUNCTION();

	if (m_paused)
		return;

	updateDragging(dt);
	processExplosionQueue();

	// slower physics if fps is too low
	dt = PxClamp(dt, 0.0f, 0.0333f);

	{
		PROFILER_SCOPED("PhysX simulate call");
		if (m_useFixedTimeStep)
		{
			m_timeAccumulator += dt;
			m_substepCount = (uint32_t)std::floor(m_timeAccumulator / m_fixedTimeStep);
			m_timeAccumulator -= m_fixedTimeStep * m_substepCount;
			m_substepCount = m_maxSubstepCount > 0 ? physx::PxClamp<uint32_t>(m_substepCount, 0, m_maxSubstepCount) : m_substepCount;
			if (m_substepCount > 0)
			{
				m_physicsScene->simulate(m_fixedTimeStep);
				m_isSimulating = true;
			}
		}
		else
		{
			m_substepCount = 1;
			PX_ASSERT(!m_isSimulating);
			m_physicsScene->simulate(dt);
			m_isSimulating = true;

		}
	}
}

void PhysXController::simualtionSyncEnd()
{
	PROFILER_SCOPED_FUNCTION();

	if (m_isSimulating)
	{
		steady_clock::time_point start = steady_clock::now();
		m_physicsScene->fetchResults(true);

		// For fixed time step case it could be that we need more then one step (m_maxSubstepCount > 1). We will run leftover steps synchronously right there.
		// Ideally is to make them overlap with other logic too, but it's much harder and requires more synchronization logic. Don't want to obfuscate sample code.
		if (m_useFixedTimeStep && m_substepCount > 1)
		{
			for (uint32_t i = 0; i < m_substepCount - 1; i++)
			{
				m_physicsScene->simulate(m_fixedTimeStep);
				m_physicsScene->fetchResults(true);
			}
		}
		m_lastSimulationTime = duration_cast<microseconds>(steady_clock::now() - start).count() * 0.000001;

		m_isSimulating = false;

		updateActorTransforms();

		PROFILER_BEGIN("Debug Render Buffer");
		getRenderer().queueRenderBuffer(&m_physicsScene->getRenderBuffer());
		PROFILER_END();
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Dragging
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void PhysXController::setDraggingEnabled(bool enabled)
{
	m_draggingEnabled = enabled;

	if (!m_draggingEnabled)
	{
		m_draggingActor = nullptr;
	}
}

void PhysXController::updateDragging(double dt)
{
	PROFILER_SCOPED_FUNCTION();

	// If dragging actor was recently removed we try to reconnect to new one once, using previous hook world point.
	// Often it is removed because it was split into smaller chunks (actors), so we wont to stay connected for nicer user experience.
	if (m_draggingActor == nullptr && m_draggingTryReconnect)
	{
		class OverlapCallback : public PxOverlapBufferN<32>
		{
		public:
			OverlapCallback() : hitActor(nullptr) {}

			PxAgain processTouches(const PxOverlapHit* buffer, PxU32 nbHits)
			{
				for (PxU32 i = 0; i < nbHits; ++i)
				{
					PxRigidDynamic* rigidDynamic = buffer[i].actor->is<PxRigidDynamic>();
					if (rigidDynamic)
					{
						hitActor = rigidDynamic;
						break;
					}
				}
				return true;
			}

			PxRigidDynamic* hitActor;
		};

		OverlapCallback overlapCallback;
		PxSphereGeometry sphere(0.15f);
		bool isHit = getPhysXScene().overlap(sphere, PxTransform(m_draggingActorLastHookWorldPoint), overlapCallback, PxQueryFilterData(PxQueryFlag::eDYNAMIC));
		if (isHit && overlapCallback.hitActor)
		{
			m_draggingActor = overlapCallback.hitActor;
		}

		m_draggingTryReconnect = false;
	}

	// Update dragging force and debug render (line)
	if (m_draggingEnabled && m_draggingActor != NULL)
	{
		const float DRAGGING_FORCE_FACTOR = 10.0f;
		const float DRAGGING_VELOCITY_FACTOR = 2.0f;
		PxVec3 attractionPoint = m_dragAttractionPoint;
		PxVec3 hookPoint = m_draggingActor->getGlobalPose().transform(m_draggingActorHookLocalPoint);
		m_draggingActorLastHookWorldPoint = hookPoint;
		m_dragVector = (m_dragAttractionPoint - hookPoint);
		if (!m_draggingActor->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
		{
			PxVec3 dragVeloctiy = (m_dragVector * DRAGGING_FORCE_FACTOR - DRAGGING_VELOCITY_FACTOR * m_draggingActor->getLinearVelocity()) * dt;
			PxRigidBodyExt::addForceAtLocalPos(*m_draggingActor, dragVeloctiy * m_draggingActor->getMass(), m_draggingActorHookLocalPoint, PxForceMode::eIMPULSE, true);
		}

		// debug render line
		m_dragDebugRenderBuffer.clear();
		m_dragDebugRenderBuffer.m_lines.push_back(PxDebugLine(attractionPoint, hookPoint, XMFLOAT4ToU32Color(HOOK_LINE_COLOR)));
		getRenderer().queueRenderBuffer(&m_dragDebugRenderBuffer);
	}
}

void PhysXController::resetDragging()
{
	m_draggingActor = nullptr;
}


LRESULT PhysXController::MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PROFILER_SCOPED_FUNCTION();

	if (m_draggingEnabled && (uMsg == WM_LBUTTONDOWN || uMsg == WM_MOUSEMOVE || uMsg == WM_LBUTTONUP))
	{
		float mouseX = (short)LOWORD(lParam) / getRenderer().getScreenWidth();
		float mouseY = (short)HIWORD(lParam) / getRenderer().getScreenHeight();

		PxVec3 eyePos, pickDir;
		getEyePoseAndPickDir(mouseX, mouseY, eyePos, pickDir);
		pickDir = pickDir.getNormalized();

		if (uMsg == WM_LBUTTONDOWN)
		{
			if (pickDir.magnitude() > 0)
			{
				PxRaycastHit	hit;
				PxRaycastBuffer rcBuffer(&hit, 1);
				bool isHit = getPhysXScene().raycast(eyePos, pickDir, PX_MAX_F32, rcBuffer, PxHitFlag::ePOSITION, PxQueryFilterData(PxQueryFlag::eDYNAMIC));
				if (isHit)
				{
					m_dragDistance = (eyePos - hit.position).magnitude();
					m_draggingActor = hit.actor->is<PxRigidDynamic>();
					m_draggingActorHookLocalPoint = m_draggingActor->getGlobalPose().getInverse().transform(hit.position);
					m_dragAttractionPoint = hit.position;
					if (!m_draggingActor->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
					{
						m_draggingActor->setLinearVelocity(PxVec3(0, 0, 0));
						m_draggingActor->setAngularVelocity(PxVec3(0, 0, 0));
					}
				}
			}
		}
		else if (uMsg == WM_MOUSEMOVE)
		{
			PxRaycastHit	hit;
			PxRaycastBuffer rcBuffer(&hit, 1);
			bool isHit = getPhysXScene().raycast(eyePos, pickDir, PX_MAX_F32, rcBuffer, PxHitFlag::ePOSITION, PxQueryFilterData(PxQueryFlag::eSTATIC));
			if (isHit)
			{
				m_dragDistance = PxMin(m_dragDistance, (eyePos - hit.position).magnitude());
			}

			m_dragAttractionPoint = eyePos + pickDir * m_dragDistance;
		}
		else if (uMsg == WM_LBUTTONUP)
		{
			m_draggingActor = NULL;
		}
	}

	return 1;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Explosion
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ExplodeOverlapCallback : public PxOverlapCallback
{
public:
	ExplodeOverlapCallback(PxVec3 worldPos, float radius, float explosiveImpulse)
		: m_worldPos(worldPos)
		, m_radius(radius)
		, m_explosiveImpulse(explosiveImpulse)
		, PxOverlapCallback(m_hitBuffer, sizeof(m_hitBuffer) / sizeof(m_hitBuffer[0])) {}

	PxAgain processTouches(const PxOverlapHit* buffer, PxU32 nbHits)
	{
		for (PxU32 i = 0; i < nbHits; ++i)
		{
			PxRigidActor* actor = buffer[i].actor;
			PxRigidDynamic* rigidDynamic = actor->is<PxRigidDynamic>();
			if (rigidDynamic && !(rigidDynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC))
			{
				if (m_actorBuffer.find(rigidDynamic) == m_actorBuffer.end())
				{
					m_actorBuffer.insert(rigidDynamic);
					PxVec3 dr = rigidDynamic->getGlobalPose().transform(rigidDynamic->getCMassLocalPose()).p - m_worldPos;
					float distance = dr.magnitude();
					float factor = PxClamp(1.0f - (distance * distance) / (m_radius * m_radius), 0.0f, 1.0f);
					float impulse = factor * m_explosiveImpulse * 1000.0f;
					PxVec3 vel = dr.getNormalized() * impulse / rigidDynamic->getMass();
					rigidDynamic->setLinearVelocity(rigidDynamic->getLinearVelocity() + vel);
				}
			}
		}
		return true;
	}

private:
	PxOverlapHit					m_hitBuffer[1000];
	float							m_explosiveImpulse;
	std::set<PxRigidDynamic*>		m_actorBuffer;
	PxVec3							m_worldPos;
	float							m_radius;
};

void PhysXController::explode(PxVec3 worldPos, float damageRadius, float explosiveImpulse)
{
	ExplodeOverlapCallback overlapCallback(worldPos, damageRadius, explosiveImpulse);
	m_physicsScene->overlap(PxSphereGeometry(damageRadius), PxTransform(worldPos), overlapCallback);
}

void PhysXController::explodeDelayed(PxVec3 worldPos, float damageRadius, float explosiveImpulse)
{
	m_explosionQueue.push_back({ worldPos, damageRadius, explosiveImpulse });
}

void PhysXController::processExplosionQueue()
{
	for (auto& e : m_explosionQueue)
	{
		explode(e.worldPos, e.damageRadius, e.explosiveImpulse);
	}
	m_explosionQueue.clear();
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//														UI
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void PhysXController::drawUI()
{
	ImGui::Checkbox("Use Fixed Timestep", &m_useFixedTimeStep);
	if (m_useFixedTimeStep)
	{
		ImGui::InputFloat("Fixed Timestep", &m_fixedTimeStep);
		ImGui::InputInt("Max Substep Count", &m_maxSubstepCount);
	}

	ImGui::Text("Substep Count:     %d", m_substepCount);
	ImGui::Text("Sync Simulation Time (total):        %4.2f ms", getLastSimulationTime() * 1000);
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												  PhysX Primitive
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void PhysXController::initPhysXPrimitives()
{
	// physx primitive render materials
	{
		m_physXPrimitiveRenderMaterial = new RenderMaterial(getRenderer().getResourceManager(), "physx_primitive", "");
		m_physXPlaneRenderMaterial = new RenderMaterial(getRenderer().getResourceManager(), "physx_primitive_plane", "");
		m_physXPrimitiveTransparentRenderMaterial = new RenderMaterial(getRenderer().getResourceManager(), "physx_primitive_transparent", "", RenderMaterial::BLEND_ALPHA_BLENDING);
	}

	// create plane
	Actor* plane = spawnPhysXPrimitivePlane(PxPlane(PxVec3(0, 1, 0).getNormalized(), 0));
	plane->setColor(PLANE_COLOR);
}

void PhysXController::releasePhysXPrimitives()
{
	// remove all actors
	for (std::set<Actor*>::iterator it = m_actors.begin(); it != m_actors.end(); it++)
	{
		delete (*it);
	}
	m_actors.clear();

	// remove all materials
	SAFE_DELETE(m_physXPrimitiveRenderMaterial);
	SAFE_DELETE(m_physXPlaneRenderMaterial);
	SAFE_DELETE(m_physXPrimitiveTransparentRenderMaterial);

	// remove all convex render meshes
	for (auto it = m_convexRenderMeshes.begin(); it != m_convexRenderMeshes.end(); it++)
	{
		SAFE_DELETE((*it).second);
	}
	m_convexRenderMeshes.clear();
}

void PhysXController::updateActorTransforms()
{
	PROFILER_SCOPED_FUNCTION();

	for (std::set<Actor*>::iterator it = m_actors.begin(); it != m_actors.end(); it++)
	{
		(*it)->update();
	}
}

PhysXController::Actor* PhysXController::spawnPhysXPrimitiveBox(const PxTransform& position, PxVec3 extents, float density)
{
	PxBoxGeometry geom = PxBoxGeometry(extents);
	PxRigidDynamic* actor = PxCreateDynamic(*m_physics, position, geom, *m_defaultMaterial, density);

	return spawnPhysXPrimitive(actor);
}

PhysXController::Actor* PhysXController::spawnPhysXPrimitivePlane(const PxPlane& plane)
{
	PxRigidStatic* actor = PxCreatePlane(*m_physics, plane, *m_defaultMaterial);
	PhysXController::Actor* p = spawnPhysXPrimitive(actor, true, true);
	return p;
}

PhysXController::Actor* PhysXController::spawnPhysXPrimitive(PxRigidActor* actor, bool addToScene, bool ownPxActor)
{
	if (addToScene)
	{
		m_physicsScene->addActor(*actor);
	}

	Actor* a = new Actor(this, actor, ownPxActor);

	m_actors.emplace(a);

	return a;
}

void PhysXController::removePhysXPrimitive(Actor* actor)
{
	if (m_actors.find(actor) == m_actors.end())
		return;

	m_actors.erase(actor);

	if (!actor->ownsPxActor())
	{
		m_physXActorsToRemove.push_back(actor->getActor());
	}

	if (m_draggingActor == actor->getActor())
	{
		m_draggingActor = nullptr;
	}

	delete actor;
}

void PhysXController::removeUnownedPhysXActors()
{
	if (m_physXActorsToRemove.size())
	{
		m_physicsScene->removeActors(m_physXActorsToRemove.data(), (PxU32)m_physXActorsToRemove.size());
		for (size_t i = 0; i < m_physXActorsToRemove.size(); ++i)
		{
			m_physXActorsToRemove[i]->release();
		}
		m_physXActorsToRemove.resize(0);
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Actor
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PhysXController::Actor::Actor(PhysXController* controller, PxRigidActor* actor, bool ownPxActor) :
	m_controller(controller),
	m_ownPxActor(ownPxActor),
	m_hidden(false)
{
	m_actor = actor;

	uint32_t shapesCount = actor->getNbShapes();
	m_shapes.resize(shapesCount);
	actor->getShapes(m_shapes.data(), shapesCount);

	m_renderables.resize(m_shapes.size());
	for (uint32_t i = 0; i < m_shapes.size(); i++)
	{
		PxShape* shape = m_shapes[i];
		IRenderMesh* mesh = m_controller->getRenderMeshForShape(shape);
		RenderMaterial* material = shape->getGeometryType() == PxGeometryType::ePLANE ? m_controller->m_physXPlaneRenderMaterial : m_controller->m_physXPrimitiveRenderMaterial;
		m_renderables[i] = m_controller->getRenderer().createRenderable(*mesh, *material);
		m_renderables[i]->setScale(m_controller->getMeshScaleForShape(shape));
	}
}

PhysXController::Actor::~Actor()
{
	for (uint32_t i = 0; i < m_renderables.size(); i++)
	{
		m_controller->getRenderer().removeRenderable(m_renderables[i]);
	}
	if (m_ownPxActor)
	{
		m_actor->release();
	}
}

void PhysXController::Actor::setColor(DirectX::XMFLOAT4 color)
{
	m_color = color;

	for (uint32_t i = 0; i < m_renderables.size(); i++)
	{
		m_renderables[i]->setColor(color);
	}
}

void PhysXController::Actor::setHidden(bool hidden)
{
	m_hidden = hidden;

	for (uint32_t i = 0; i < m_renderables.size(); i++)
	{
		m_renderables[i]->setHidden(hidden);
	}
}

void PhysXController::Actor::update()
{
	for (uint32_t i = 0; i < m_renderables.size(); i++)
	{
		m_renderables[i]->setTransform(m_actor->getGlobalPose() * m_shapes[i]->getLocalPose());
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												PhysX Shapes Renderer
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IRenderMesh* PhysXController::getConvexRenderMesh(const PxConvexMesh* mesh)
{
	auto it = m_convexRenderMeshes.find(mesh);
	if (it != m_convexRenderMeshes.end())
	{
		return (*it).second;
	}
	else
	{
		ConvexRenderMesh* renderMesh = new ConvexRenderMesh(mesh);
		m_convexRenderMeshes[mesh] = renderMesh;
		return renderMesh;
	}
}

IRenderMesh* PhysXController::getRenderMeshForShape(const PxShape* shape)
{
	switch (shape->getGeometryType())
	{
	case PxGeometryType::eBOX:
		return getRenderer().getPrimitiveRenderMesh(PrimitiveRenderMeshType::Box);
	case PxGeometryType::ePLANE:
		return getRenderer().getPrimitiveRenderMesh(PrimitiveRenderMeshType::Plane);
	case PxGeometryType::eSPHERE:
		return getRenderer().getPrimitiveRenderMesh(PrimitiveRenderMeshType::Sphere);
	case PxGeometryType::eCONVEXMESH:
	{
		PxConvexMeshGeometry geom;
		shape->getConvexMeshGeometry(geom);
		return getConvexRenderMesh(geom.convexMesh);
	}
	default:
		PX_ALWAYS_ASSERT_MESSAGE("Unsupported PxGeometryType");
		return NULL;
	}
}

PxVec3 PhysXController::getMeshScaleForShape(const PxShape* shape)
{
	switch (shape->getGeometryType())
	{
	case PxGeometryType::eBOX:
	{
		PxBoxGeometry boxGeom;
		shape->getBoxGeometry(boxGeom);
		return boxGeom.halfExtents;
	}
	case PxGeometryType::ePLANE:
	{
		return PxVec3(1, 2000, 2000);
	}
	case PxGeometryType::eSPHERE:
	{
		PxSphereGeometry sphereGeom;
		shape->getSphereGeometry(sphereGeom);
		return PxVec3(sphereGeom.radius, sphereGeom.radius, sphereGeom.radius);
	}
	case PxGeometryType::eCONVEXMESH:
	{
		PxConvexMeshGeometry convexGeom;
		shape->getConvexMeshGeometry(convexGeom);
		return convexGeom.scale.scale; // maybe incorrect because of rotation not used
	}
	default:
		return PxVec3(1, 1, 1);
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Utils
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PxVec3 unproject(PxMat44& proj, PxMat44& view, float x, float y)
{
	PxVec4 screenPoint(x, y, 0, 1);
	PxVec4 viewPoint = PxVec4(x / proj[0][0], y / proj[1][1], 1, 1);
	PxVec4 nearPoint = view.inverseRT().transform(viewPoint);
	if (nearPoint.w)
		nearPoint *= 1.0f / nearPoint.w;
	return PxVec3(nearPoint.x, nearPoint.y, nearPoint.z);
}


void PhysXController::getEyePoseAndPickDir(float mouseX, float mouseY, PxVec3& eyePos, PxVec3& pickDir)
{
	PxMat44 view = XMMATRIXToPxMat44(getRenderer().getCamera().GetViewMatrix());
	PxMat44 proj = XMMATRIXToPxMat44(getRenderer().getCamera().GetProjMatrix());

	PxMat44 eyeTransform = view.inverseRT();
	eyePos = eyeTransform.getPosition();
	PxVec3 nearPos = unproject(proj, view, mouseX * 2 - 1, 1 - mouseY * 2);
	pickDir = nearPos - eyePos;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
