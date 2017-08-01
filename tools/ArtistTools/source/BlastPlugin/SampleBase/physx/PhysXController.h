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


#ifndef PHYSX_CONTROLLER_H
#define PHYSX_CONTROLLER_H

#include "SampleManager.h"
#include <DirectXMath.h>
#include "DebugRenderBuffer.h"
#include "PxFiltering.h"
#include <set>
#include <map>
// Add By Lixu Begin
#include "BlastModel.h"
// Add By Lixu End


using namespace physx;

class PerformanceDataWriter;
class RenderMaterial;
class Renderable;
class IRenderMesh;
// Add By Lixu Begin
class SimpleMesh;
// Add By Lixu End

namespace physx
{
class PxCpuDispatcher;
class PxFoundation;
class PxPhysics;
class PxCooking;
class PxPvd;
class PxCudaContextManager;
class PxDefaultCpuDispatcher;
}


/**
SampleController which manages all the PhysX related work:
1. initialization, scene updates, release.
2. it can create update and render physx primitives. They are represented by PhysXController::Actor, see public API.
3. provides ability to drag actors by mouse or other similar input

NOTE: this class does too much, probably should be split in a few smaller ones.
*/
class PhysXController : public ISampleController
{
  public:

	//////// Actor ////////

	class Actor
	{
	public:

		Actor(PhysXController* controller, PxRigidActor* actor, bool ownPxActor = true);
		~Actor();

		void setColor(DirectX::XMFLOAT4 color);
		DirectX::XMFLOAT4 getColor() const { return m_color; }

		bool isHidden() { return m_hidden; }
		void setHidden(bool hidden);

		void update();
		PxRigidActor* getActor() const { return m_actor; }

		bool ownsPxActor() const { return m_ownPxActor; }

	private:
		PhysXController*      m_controller;
		PxRigidActor*         m_actor;
		std::vector<PxShape*> m_shapes;

		std::vector<Renderable*>   m_renderables;
		DirectX::XMFLOAT4     m_color;

		bool                  m_hidden;
		bool                  m_ownPxActor;
	};


	//////// ctor ////////

	PhysXController(PxSimulationFilterShader filterShader);
	virtual ~PhysXController();


	//////// virtual callbacks ////////

	virtual void onInitialize() override;
	virtual void onTerminate() override;

	virtual LRESULT MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


	//////// public API ////////

	void simulationBegin(float dt);
	void simualtionSyncEnd();

	void getEyePoseAndPickDir(float mouseX, float mouseY, PxVec3& eyePos, PxVec3& pickDir);

	// wrappers to physx calls
	PxRigidDynamic* createRigidDynamic(const PxTransform& transform);
	void			releaseRigidDynamic(PxRigidDynamic*);

	Actor*	spawnPhysXPrimitiveBox(const PxTransform& position, PxVec3 extents = PxVec3(1, 1, 1), float density = 2000.0f);
	Actor*	spawnPhysXPrimitivePlane(const PxPlane& plane);
	Actor*	spawnPhysXPrimitive(PxRigidActor* actor, bool addToScene = true, bool ownPxActor = true);
	void	removePhysXPrimitive(Actor*);

	IRenderMesh* getConvexRenderMesh(const PxConvexMesh* mesh);
	IRenderMesh* getRenderMeshForShape(const PxShape* shape);
	PxVec3		 getMeshScaleForShape(const PxShape* shape);

	void removeUnownedPhysXActors();

	bool isPaused() const
	{
		return m_paused;
	}

	void setPaused(bool paused)
	{
		m_paused = paused;
	}

	void setDraggingEnabled(bool enabled);
	bool getDraggingEnabled() const { return m_draggingEnabled; }
	void resetDragging();

	void notifyRigidDynamicDestroyed(PxRigidDynamic*);

	void explode(PxVec3 worldPos, float damageRadius, float explosiveImpulse);
	void explodeDelayed(PxVec3 worldPos, float damageRadius, float explosiveImpulse);

	void drawUI();

	//////// public getters ////////

	double getLastSimulationTime() const
	{
		return m_lastSimulationTime;
	}

	RenderMaterial* getPrimitiveRenderMaterial() 
	{ 
		return m_physXPrimitiveRenderMaterial; 
	}

// Add By Lixu Begin
	bool ExportCollisionRepX(const char* fname, physx::PxPhysics* pSDK, physx::PxScene* pScene, bool asBinary);
	void ClearOldCOllisions();
	RenderMaterial* getFBXRenderMaterial()
	{
		return m_physXPlaneRenderMaterial;
	}
	bool m_bFirstTime;
	bool isPlaneVisible();
	void setPlaneVisible(bool bVisible);

	PxScene& getEditPhysXScene() const
	{
		return *m_editPhysicsScene;
	}

	PxRigidDynamic* createEditPhysXActor(const std::vector<BlastModel::Chunk::Mesh>& meshes, const PxTransform& pos);
// Add By Lixu End

	PxPhysics& getPhysics() const
	{
		return *m_physics;
	}

	PxScene& getPhysXScene() const
	{
		return *m_physicsScene;
	}

	PxMaterial* getDefaultMaterial() const
	{
		return m_defaultMaterial;
	}

	PxCooking& getCooking() const
	{
		return *m_cooking;
	}

	PxDefaultCpuDispatcher* getCPUDispatcher() const
	{
		return m_dispatcher;
	}

	void setPerformanceWriter(PerformanceDataWriter* perfWriter)
	{
		m_perfWriter = perfWriter;
	}

	bool getGPUPhysicsAvailable() const
	{
		return m_gpuPhysicsAvailable;
	}

	void setUseGPUPhysics(bool useGPUPhysics);

	bool getUseGPUPhysics() const
	{
		return m_useGPUPhysics;
	}

	const PxVec3& getDragActorHookLocalPoint() const
	{
		return m_draggingActorHookLocalPoint;
	}

	const PxVec3& getDragVector() const
	{
		return m_dragVector;
	}

	PxRigidDynamic* getDraggingActor() const
	{
		return m_draggingActor;
	}

  private:
	//////// internal methods ////////

	void initPhysX();
	void releasePhysX();

	void initPhysXPrimitives();
	void releasePhysXPrimitives();
	void updateActorTransforms();
	void updateDragging(double dt);
	void processExplosionQueue();

	
	//////// used controllers ////////

	Renderer& getRenderer() const
	{
		return getManager()->getRenderer();
	}


	//////// internal data ////////

	// PhysX 
	PxFoundation*                               m_foundation;
	PxPhysics*                                  m_physics;
	PxCooking*                                  m_cooking;
	PxPvd*                                      m_pvd;
	PxCudaContextManager*                       m_cudaContext;
	PxDefaultCpuDispatcher*                     m_dispatcher;
	PxMaterial*                                 m_defaultMaterial;
	PxSimulationFilterShader                    m_filterShader;
	PxScene*                                    m_physicsScene;
	// Add By Lixu Begin
	PxScene*                                    m_editPhysicsScene;
	// Add By Lixu End

	// PhysX API related
	std::vector<PxActor*>                       m_physXActorsToRemove;

	// primitives/actors
	std::set<Actor*>                            m_actors;
	std::map<const PxConvexMesh*, IRenderMesh*> m_convexRenderMeshes;
	RenderMaterial*                             m_physXPrimitiveRenderMaterial;
	RenderMaterial*                             m_physXPlaneRenderMaterial;
	RenderMaterial*                             m_physXPrimitiveTransparentRenderMaterial;

	// simulation
	bool										m_isSimulating;
	bool										m_gpuPhysicsAvailable;
	bool										m_useGPUPhysics;
	double                                      m_lastSimulationTime;
	LARGE_INTEGER                               m_performanceFreq;
	bool                                        m_paused;
	bool										m_useFixedTimeStep;
	float										m_fixedTimeStep;
	float										m_timeAccumulator;
	uint32_t									m_substepCount;
	int32_t										m_maxSubstepCount;

	// dragging
	bool                                        m_draggingEnabled;
	PxRigidDynamic*                             m_draggingActor;
	PxVec3                                      m_draggingActorHookLocalPoint;
	PxVec3                                      m_dragAttractionPoint;
	PxVec3                                      m_dragVector;
	float                                       m_dragDistance;
	DebugRenderBuffer                           m_dragDebugRenderBuffer;
	PxVec3										m_draggingActorLastHookWorldPoint;
	bool										m_draggingTryReconnect;

	// Performance writer
	PerformanceDataWriter*						m_perfWriter;

	// explosion
	struct ExplosionData
	{
		PxVec3 worldPos;
		float damageRadius;
		float explosiveImpulse;
	};

	std::vector<ExplosionData>					m_explosionQueue;

};

#endif
