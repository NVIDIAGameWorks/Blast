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


#ifndef GIZMO_TOOL_CONTROLLER_H
#define GIZMO_TOOL_CONTROLLER_H

#include "SampleManager.h"
#include "DebugRenderBuffer.h"
#include "NvBlastExtPxManager.h"
#include "BlastSceneTree.h"

class Renderable;
class RenderMaterial;

namespace Nv
{
namespace Blast
{
class ExtPhysicsActor;
}
}

namespace physx
{
	class PxScene;
}

enum AxisType
{
	AT_X = 0,
	AT_Y = 1,
	AT_Z = 2,
	AT_Num
};

enum GizmoToolMode
{
	GTM_Translate = 0,
	GTM_Scale,
	GTM_Rotation
};

void modifyPxActorByLocalWay(PxScene& pxScene, PxRigidDynamic& actor, PxTransform& gp_old, PxTransform& gp_new);
void scalePxActor(PxScene& pxScene, PxRigidDynamic& actor, PxMat44& scale);

class GizmoToolController : public ISampleController, public ISceneObserver
{
public:
	GizmoToolController();
	virtual ~GizmoToolController();

	virtual LRESULT MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void Animate(double dt);
	void drawUI();

	virtual void onInitialize();
	virtual void onSampleStart();
	virtual void onSampleStop();

	virtual void dataSelected(std::vector<BlastNode*> selections);

	void setGizmoToolMode(GizmoToolMode mode);
	GizmoToolMode getGizmoToolMode() { return m_GizmoToolMode; }
	void setAxisSelected(AxisType type);
	void setAxisLength(float axisLength);
	void showAxisRenderables(bool show);
	void resetPos();
	void setTargetActor(PxActor* actor);
	PxActor* getTargetActor();
	void syncRenderableState();
	bool CanMapToRootChunk(PxActor* actor);

private:
	GizmoToolController& operator= (GizmoToolController&);

	//////// private methods ////////

	physx::PxScene& GetPhysXScene();
	void UpdateCircleRenderData(float axisLength);
	bool CalPlaneLineIntersectPoint(PxVec3& result, PxVec3 planeNormal, PxVec3 planePoint, PxVec3 linedirection, PxVec3 linePoint);
	float DistanceFromPointToLine(PxVec3& point, PxVec3& origin, PxVec3& direction, PxVec3& foot);
	bool GetFootFromPointToPlane(PxVec3& point, PxVec3& origin, PxVec3& normal, PxVec3& foot);
	bool GetIntersectBetweenLines(PxVec3& origin1, PxVec3& direction1, PxVec3& origin2, PxVec3& direction2, PxVec3& intersect);
	PxQuat CalDirectionQuat(AxisType type);
	PxQuat CalConvertQuat();
	void ScaleActor(bool replace);
	
	bool CanModifyLocal(PxActor* actor);
	void UpdateAssetInstanceTransform(const PxTransform& position);

	//////// used controllers ////////

	Renderer& getRenderer() const
	{
		return getManager()->getRenderer();
	}

	PhysXController& getPhysXController() const
	{
		return getManager()->getPhysXController();
	}

	BlastController& getBlastController() const
	{
		return getManager()->getBlastController();
	}

	SelectionToolController& getSelectionToolController() const
	{
		return getManager()->getSelectionToolController();
	}

	//////// internal data ////////

	GizmoToolMode m_GizmoToolMode;

	bool m_bGizmoFollowed;
	PxVec3 m_LastEyeRay;
	PxVec3 m_LastFoot;
	PxVec3 m_LastAxis[AT_Num];

	PxRigidDynamic* m_CurrentActor;

	bool m_bNeedResetPos;
	bool m_bNeedResetColor;

	PxVec3 m_TargetPos;
	PxVec3 m_Axis[AT_Num];
	AxisType m_AxisSelected;
	Renderable* m_AxisConeRenderable[AT_Num];
	Renderable* m_AxisBoxRenderable[AT_Num];

	RenderMaterial* m_AxisRenderMaterial;
	DebugRenderBuffer m_AxisRenderBuffer;

	std::vector<PxDebugLine> m_CircleRenderData;
	DebugRenderBuffer m_CircleRenderBuffer;

	std::vector<BlastAssetInstanceNode*> m_assetInstances;
};

#endif // GIZMO_TOOL_CONTROLLER_H