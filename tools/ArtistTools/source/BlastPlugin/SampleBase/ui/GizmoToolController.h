/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef GIZMO_TOOL_CONTROLLER_H
#define GIZMO_TOOL_CONTROLLER_H

#include "SampleManager.h"
#include "DebugRenderBuffer.h"
#include "NvBlastExtPxManager.h"

class Renderable;
class RenderMaterial;

namespace Nv
{
namespace Blast
{
class ExtPhysicsActor;
}
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

class GizmoToolController : public ISampleController
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

	void setGizmoToolMode(GizmoToolMode mode);
	void setAxisSelected(AxisType type);
	void showAxisRenderables(bool show);
	void resetPos();

private:
	GizmoToolController& operator= (GizmoToolController&);

	//////// private methods ////////

	bool CalPlaneLineIntersectPoint(PxVec3& result, PxVec3 planeNormal, PxVec3 planePoint, PxVec3 linedirection, PxVec3 linePoint);
	float DistanceFromPointToLine(PxVec3& point, PxVec3& origin, PxVec3& direction, PxVec3& foot);
	bool GetFootFromPointToPlane(PxVec3& point, PxVec3& origin, PxVec3& normal, PxVec3& foot);
	bool GetIntersectBetweenLines(PxVec3& origin1, PxVec3& direction1, PxVec3& origin2, PxVec3& direction2, PxVec3& intersect);
	PxQuat CalDirectionQuat(AxisType type);
	PxQuat CalConvertQuat();
	void ScaleActor(bool replace);
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

	PxRigidActor* m_CurrentActor;

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
};

#endif // GIZMO_TOOL_CONTROLLER_H