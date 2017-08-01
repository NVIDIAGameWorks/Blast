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


#include "GizmoToolController.h"
#include "RenderUtils.h"
#include "BlastController.h"
#include "SelectionToolController.h"
#include "Renderer.h"
#include "PhysXController.h"
#include "SampleProfiler.h"
#include "ViewerOutput.h"

#include <imgui.h>

#include "NvBlastTkActor.h"
#include "NvBlastExtDamageShaders.h"

#include "PxRigidDynamic.h"
#include "PxScene.h"
#include "PxPhysics.h"
#include "cooking/PxCooking.h"
#include "NvBlastExtPxActor.h"
#include <AppMainWindow.h>
#include "BlastSceneTree.h"
#include "SimpleScene.h"
using namespace Nv::Blast;
using namespace physx;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Setup
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const DirectX::XMFLOAT4 X_DIRECTION_COLOR_F = DirectX::XMFLOAT4(1, 0, 0, 1);
const DirectX::XMFLOAT4 Y_DIRECTION_COLOR_F = DirectX::XMFLOAT4(0, 1, 0, 1);
const DirectX::XMFLOAT4 Z_DIRECTION_COLOR_F = DirectX::XMFLOAT4(0, 0, 1, 1);
const DirectX::XMFLOAT4 HIGHLIGHT_COLOR_F = DirectX::XMFLOAT4(1, 1, 0, 1);

const physx::PxU32 X_DIRECTION_COLOR_U = XMFLOAT4ToU32Color(X_DIRECTION_COLOR_F);
const physx::PxU32 Y_DIRECTION_COLOR_U = XMFLOAT4ToU32Color(Y_DIRECTION_COLOR_F);
const physx::PxU32 Z_DIRECTION_COLOR_U = XMFLOAT4ToU32Color(Z_DIRECTION_COLOR_F);
const physx::PxU32 HIGHLIGHT_COLOR_U = XMFLOAT4ToU32Color(HIGHLIGHT_COLOR_F);

float defaultAxisLength = 10.0;
const float defaultAxisModifier = -1.0;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void modifyPxActorByLocalWay(PxScene& pxScene, PxRigidDynamic& actor, PxTransform& gp_old, PxTransform& gp_new)
{
	uint32_t shapesCount = actor.getNbShapes();
	if (shapesCount > 0)
	{
		PxTransform gp_newInv = gp_new.getInverse();

		PxTransform lp_old;
		PxTransform lp_new;

		std::vector<PxShape*> shapes(shapesCount);
		actor.getShapes(&shapes[0], shapesCount);

		pxScene.removeActor(actor);
		for (uint32_t i = 0; i < shapesCount; i++)
		{
			PxShape* shape = shapes[i];

			actor.detachShape(*shape);

			lp_old = shape->getLocalPose();
			lp_new = gp_newInv * gp_old * lp_old;
			shape->setLocalPose(lp_new);

			actor.attachShape(*shape);
		}
		pxScene.addActor(actor);
	}
}

void scalePxActor(PxScene& pxScene, PxRigidDynamic& actor, PxMat44& scale)
{
	uint32_t shapesCount = actor.getNbShapes();
	if (shapesCount == 0)
	{
		return;
	}

	std::vector<PxShape*> shapes(shapesCount);
	actor.getShapes(&shapes[0], shapesCount);

	pxScene.removeActor(actor);

	for (uint32_t i = 0; i < shapesCount; i++)
	{
		PxShape* shape = shapes[i];

		PxConvexMeshGeometry mesh;
		bool valid = shape->getConvexMeshGeometry(mesh);
		if (!valid)
		{
			continue;
		}

		PxConvexMesh* pMesh = mesh.convexMesh;
		if (NULL == pMesh)
		{
			continue;
		}

		PxU32 numVertex = pMesh->getNbVertices();
		if (numVertex == 0)
		{
			continue;
		}

		const PxVec3* pVertex = pMesh->getVertices();
		PxVec3* pVertexNew = new PxVec3[numVertex];
		for (PxU32 v = 0; v < numVertex; v++)
		{
			pVertexNew[v] = scale.transform(pVertex[v]);
		}

		PxConvexMeshDesc convexMeshDesc;
		convexMeshDesc.points.count = numVertex;
		convexMeshDesc.points.data = pVertexNew;
		convexMeshDesc.points.stride = sizeof(PxVec3);
		convexMeshDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;
		SampleManager* manager = SampleManager::ins();
		PxPhysics& physics = manager->getPhysXController().getPhysics();
		PxCooking& cooking = manager->getPhysXController().getCooking();
		PxConvexMesh* convexMesh = cooking.createConvexMesh(convexMeshDesc, physics.getPhysicsInsertionCallback());
		if (NULL == convexMesh)
		{
			delete[] pVertexNew;
			continue;
		}

		mesh.convexMesh = convexMesh;

		actor.detachShape(*shape);
		shape->setGeometry(mesh);
		actor.attachShape(*shape);

		pMesh->release();
		delete[] pVertexNew;
	}

	pxScene.addActor(actor);
}

GizmoToolController::GizmoToolController()
{
	m_bGizmoFollowed = false;

	UpdateCircleRenderData(defaultAxisLength);

	resetPos();

	BlastSceneTree::ins()->addObserver(this);
}

GizmoToolController::~GizmoToolController()
{
}

void GizmoToolController::onSampleStart()
{
	m_AxisRenderMaterial = new RenderMaterial("", getRenderer().getResourceManager(), "physx_primitive_transparent");

	IRenderMesh* coneMesh = getRenderer().getPrimitiveRenderMesh(PrimitiveRenderMeshType::Cone);
	m_AxisConeRenderable[AT_X] = getRenderer().createRenderable(*coneMesh, *m_AxisRenderMaterial);
	m_AxisConeRenderable[AT_Y] = getRenderer().createRenderable(*coneMesh, *m_AxisRenderMaterial);
	m_AxisConeRenderable[AT_Z] = getRenderer().createRenderable(*coneMesh, *m_AxisRenderMaterial);
	m_AxisConeRenderable[AT_X]->setColor(X_DIRECTION_COLOR_F);
	m_AxisConeRenderable[AT_Y]->setColor(Y_DIRECTION_COLOR_F);
	m_AxisConeRenderable[AT_Z]->setColor(Z_DIRECTION_COLOR_F);
	m_AxisConeRenderable[AT_X]->setScale(PxVec3(0.5, 1, 0.5));
	m_AxisConeRenderable[AT_Y]->setScale(PxVec3(0.5, 1, 0.5));
	m_AxisConeRenderable[AT_Z]->setScale(PxVec3(0.5, 1, 0.5));
	m_AxisConeRenderable[AT_X]->setHidden(true);
	m_AxisConeRenderable[AT_Y]->setHidden(true);
	m_AxisConeRenderable[AT_Z]->setHidden(true);

	IRenderMesh* boxMesh = getRenderer().getPrimitiveRenderMesh(PrimitiveRenderMeshType::Box);
	m_AxisBoxRenderable[AT_X] = getRenderer().createRenderable(*boxMesh, *m_AxisRenderMaterial);
	m_AxisBoxRenderable[AT_Y] = getRenderer().createRenderable(*boxMesh, *m_AxisRenderMaterial);
	m_AxisBoxRenderable[AT_Z] = getRenderer().createRenderable(*boxMesh, *m_AxisRenderMaterial);
	m_AxisBoxRenderable[AT_X]->setColor(X_DIRECTION_COLOR_F);
	m_AxisBoxRenderable[AT_Y]->setColor(Y_DIRECTION_COLOR_F);
	m_AxisBoxRenderable[AT_Z]->setColor(Z_DIRECTION_COLOR_F);
	m_AxisBoxRenderable[AT_X]->setScale(PxVec3(0.5, 0.5, 0.5));
	m_AxisBoxRenderable[AT_Y]->setScale(PxVec3(0.5, 0.5, 0.5));
	m_AxisBoxRenderable[AT_Z]->setScale(PxVec3(0.5, 0.5, 0.5));
	m_AxisBoxRenderable[AT_X]->setHidden(true);
	m_AxisBoxRenderable[AT_Y]->setHidden(true);
	m_AxisBoxRenderable[AT_Z]->setHidden(true);

	m_Axis[AT_X] = PxVec3(defaultAxisLength, 0.0, 0.0);
	m_Axis[AT_Y] = PxVec3(0.0, defaultAxisLength, 0.0);
	m_Axis[AT_Z] = PxVec3(0.0, 0.0, defaultAxisLength);
}

void GizmoToolController::onInitialize()
{
}


void GizmoToolController::onSampleStop() 
{
}

void GizmoToolController::dataSelected(std::vector<BlastNode*> selections)
{
	if (!IsEnabled())
		return;

	BlastController& blastController = getBlastController();
	std::vector<BlastFamilyPtr>& families = blastController.getFamilies();

	std::set<PxActor*> selectedActors;
	PxActor* targetActor = nullptr;
	for (BlastFamily* family : families)
	{
		std::vector<uint32_t> selectedChunks = family->getSelectedChunks();
		for (uint32_t chunkIndex : selectedChunks)
		{
			PxActor* actor = nullptr;
			family->getPxActorByChunkIndex(chunkIndex, &actor);

			if (actor)
			{
				selectedActors.insert(actor);
				targetActor = actor;
			}
		}
	}

	if (targetActor)
		setTargetActor(targetActor);
	getSelectionToolController().setTargetActors(selectedActors);
}

void GizmoToolController::Animate(double dt)
{
	PROFILER_SCOPED_FUNCTION();

	if (NULL == m_CurrentActor)
	{
		m_AxisConeRenderable[AT_X]->setHidden(true);
		m_AxisConeRenderable[AT_Y]->setHidden(true);
		m_AxisConeRenderable[AT_Z]->setHidden(true);
		m_AxisBoxRenderable[AT_X]->setHidden(true);
		m_AxisBoxRenderable[AT_Y]->setHidden(true);
		m_AxisBoxRenderable[AT_Z]->setHidden(true);

		return;
	}

	m_TargetPos = m_CurrentActor->getGlobalPose().p;
	m_bNeedResetPos = true;

	bool isTranslation = m_GizmoToolMode == GTM_Translate;
	bool isScale = m_GizmoToolMode == GTM_Scale;
	bool isRotation = m_GizmoToolMode == GTM_Rotation;
	bool isLocal = AppMainWindow::Inst().m_bGizmoWithLocal;

	bool showLine = isTranslation || isScale || (isRotation && isLocal);
	bool showCone = isTranslation || (isRotation && isLocal);
	bool showBox = isScale;
	bool showCircle = isRotation;

	m_AxisConeRenderable[AT_X]->setHidden(!isTranslation);
	m_AxisConeRenderable[AT_Y]->setHidden(!isTranslation);
	m_AxisConeRenderable[AT_Z]->setHidden(!isTranslation);
	m_AxisBoxRenderable[AT_X]->setHidden(!isScale);
	m_AxisBoxRenderable[AT_Y]->setHidden(!isScale);
	m_AxisBoxRenderable[AT_Z]->setHidden(!isScale);

	syncRenderableState();

	if (showLine)
	{
		if (m_bNeedResetPos)
		{
			m_AxisRenderBuffer.clear();
			m_AxisRenderBuffer.m_lines.push_back(PxDebugLine(m_TargetPos, m_TargetPos + m_Axis[AT_X] * defaultAxisModifier, X_DIRECTION_COLOR_U));
			m_AxisRenderBuffer.m_lines.push_back(PxDebugLine(m_TargetPos, m_TargetPos + m_Axis[AT_Y], Y_DIRECTION_COLOR_U));
			m_AxisRenderBuffer.m_lines.push_back(PxDebugLine(m_TargetPos, m_TargetPos + m_Axis[AT_Z], Z_DIRECTION_COLOR_U));
		}

		if (m_bNeedResetColor)
		{
			if (m_AxisSelected == AT_X)
			{
				m_AxisRenderBuffer.m_lines[0].color0 = HIGHLIGHT_COLOR_U;
				m_AxisRenderBuffer.m_lines[0].color1 = HIGHLIGHT_COLOR_U;
			}
			else
			{
				m_AxisRenderBuffer.m_lines[0].color0 = X_DIRECTION_COLOR_U;
				m_AxisRenderBuffer.m_lines[0].color1 = X_DIRECTION_COLOR_U;
			}
			if (m_AxisSelected == AT_Y)
			{
				m_AxisRenderBuffer.m_lines[1].color0 = HIGHLIGHT_COLOR_U;
				m_AxisRenderBuffer.m_lines[1].color1 = HIGHLIGHT_COLOR_U;
			}
			else
			{
				m_AxisRenderBuffer.m_lines[1].color0 = Y_DIRECTION_COLOR_U;
				m_AxisRenderBuffer.m_lines[1].color1 = Y_DIRECTION_COLOR_U;
			}
			if (m_AxisSelected == AT_Z)
			{
				m_AxisRenderBuffer.m_lines[2].color0 = HIGHLIGHT_COLOR_U;
				m_AxisRenderBuffer.m_lines[2].color1 = HIGHLIGHT_COLOR_U;
			}
			else
			{
				m_AxisRenderBuffer.m_lines[2].color0 = Z_DIRECTION_COLOR_U;
				m_AxisRenderBuffer.m_lines[2].color1 = Z_DIRECTION_COLOR_U;
			}
		}

		getRenderer().queueRenderBuffer(&m_AxisRenderBuffer);
	}

	if (showCone)
	{
		if (m_bNeedResetPos)
		{
			PxTransform transform;

			transform.p = m_TargetPos + m_Axis[AT_X] * defaultAxisModifier;
			transform.q = CalDirectionQuat(AT_X);
			m_AxisConeRenderable[AT_X]->setTransform(transform);

			transform.p = m_TargetPos + m_Axis[AT_Y];
			transform.q = CalDirectionQuat(AT_Y);
			m_AxisConeRenderable[AT_Y]->setTransform(transform);

			transform.p = m_TargetPos + m_Axis[AT_Z];
			transform.q = CalDirectionQuat(AT_Z);
			m_AxisConeRenderable[AT_Z]->setTransform(transform);
		}

		if (m_bNeedResetColor)
		{
			if (m_AxisSelected == AT_X)
			{
				m_AxisConeRenderable[AT_X]->setColor(HIGHLIGHT_COLOR_F);
			}
			else
			{
				m_AxisConeRenderable[AT_X]->setColor(X_DIRECTION_COLOR_F);
			}
			if (m_AxisSelected == AT_Y)
			{
				m_AxisConeRenderable[AT_Y]->setColor(HIGHLIGHT_COLOR_F);
			}
			else
			{
				m_AxisConeRenderable[AT_Y]->setColor(Y_DIRECTION_COLOR_F);
			}
			if (m_AxisSelected == AT_Z)
			{
				m_AxisConeRenderable[AT_Z]->setColor(HIGHLIGHT_COLOR_F);
			}
			else
			{
				m_AxisConeRenderable[AT_Z]->setColor(Z_DIRECTION_COLOR_F);
			}
		}
	}

	if (showBox)
	{
		if (m_bNeedResetPos)
		{
			PxTransform transform;

			transform.p = m_TargetPos + m_Axis[AT_X] * defaultAxisModifier;
			transform.q = CalDirectionQuat(AT_X);
			m_AxisBoxRenderable[AT_X]->setTransform(transform);

			transform.p = m_TargetPos + m_Axis[AT_Y];
			transform.q = CalDirectionQuat(AT_Y);
			m_AxisBoxRenderable[AT_Y]->setTransform(transform);

			transform.p = m_TargetPos + m_Axis[AT_Z];
			transform.q = CalDirectionQuat(AT_Z);
			m_AxisBoxRenderable[AT_Z]->setTransform(transform);
		}

		if (m_bNeedResetColor)
		{
			if (m_AxisSelected == AT_X)
			{
				m_AxisBoxRenderable[AT_X]->setColor(HIGHLIGHT_COLOR_F);
			}
			else
			{
				m_AxisBoxRenderable[AT_X]->setColor(X_DIRECTION_COLOR_F);
			}
			if (m_AxisSelected == AT_Y)
			{
				m_AxisBoxRenderable[AT_Y]->setColor(HIGHLIGHT_COLOR_F);
			}
			else
			{
				m_AxisBoxRenderable[AT_Y]->setColor(Y_DIRECTION_COLOR_F);
			}
			if (m_AxisSelected == AT_Z)
			{
				m_AxisBoxRenderable[AT_Z]->setColor(HIGHLIGHT_COLOR_F);
			}
			else
			{
				m_AxisBoxRenderable[AT_Z]->setColor(Z_DIRECTION_COLOR_F);
			}
		}
	}

	if (showCircle)
	{
		if (m_bNeedResetPos)
		{
			PxQuat q = CalConvertQuat();

			m_CircleRenderBuffer.clear();
			std::vector<PxDebugLine>::iterator it;
			for (it = m_CircleRenderData.begin(); it != m_CircleRenderData.end(); it++)
			{
				PxDebugLine line = (*it);

				line.pos0 = q.rotate(line.pos0);
				line.pos1 = q.rotate(line.pos1);

				line.pos0 += m_TargetPos;
				line.pos1 += m_TargetPos;

				m_CircleRenderBuffer.m_lines.push_back(line);
			}
		}

		if (m_bNeedResetColor)
		{
			std::vector<PxDebugLine>& datas = m_CircleRenderData;
			std::vector<PxDebugLine>& lines = m_CircleRenderBuffer.m_lines;
			int linesize = lines.size();
			int linesize_per_axis = linesize / 3;
			float multiply;
			physx::PxU32 color;

			if (m_AxisSelected == AT_X)
			{
				for (int l = 0; l < linesize_per_axis; l++)
				{
					multiply = 1.0 * (l + 1) / linesize_per_axis;
					color = XMFLOAT4ToU32Color(DirectX::XMFLOAT4(multiply, multiply, 0, 1));
					lines[l].color0 = color;
					lines[l].color1 = color;
				}
			}
			else
			{
				for (int l = 0; l < linesize_per_axis; l++)
				{
					multiply = 1.0 * (l + 1) / linesize_per_axis;
					color = XMFLOAT4ToU32Color(DirectX::XMFLOAT4(multiply, 0, 0, 1));
					lines[l].color0 = color;
					lines[l].color1 = color;
				}
			}
			if (m_AxisSelected == AT_Y)
			{
				for (int l = linesize_per_axis; l < linesize_per_axis * 2; l++)
				{
					multiply = 1.0 * (l + 1 - linesize_per_axis) / linesize_per_axis;
					color = XMFLOAT4ToU32Color(DirectX::XMFLOAT4(multiply, multiply, 0, 1));
					lines[l].color0 = color;
					lines[l].color1 = color;
				}
			}
			else
			{
				for (int l = linesize_per_axis; l < linesize_per_axis * 2; l++)
				{
					multiply = 1.0 * (l + 1 - linesize_per_axis) / linesize_per_axis;
					color = XMFLOAT4ToU32Color(DirectX::XMFLOAT4(0, multiply, 0, 1));
					lines[l].color0 = color;
					lines[l].color1 = color;
				}
			}
			if (m_AxisSelected == AT_Z)
			{
				for (int l = linesize_per_axis * 2; l < linesize; l++)
				{
					multiply = 1.0 * (l + 1 - linesize_per_axis * 2) / linesize_per_axis;
					color = XMFLOAT4ToU32Color(DirectX::XMFLOAT4(multiply, multiply, 0, 1));
					lines[l].color0 = color;
					lines[l].color1 = color;
				}
			}
			else
			{
				for (int l = linesize_per_axis * 2; l < linesize; l++)
				{
					multiply = 1.0 * (l + 1 - linesize_per_axis * 2) / linesize_per_axis;
					color = XMFLOAT4ToU32Color(DirectX::XMFLOAT4(0, 0, multiply, 1));
					lines[l].color0 = color;
					lines[l].color1 = color;
				}
			}
		}

		getRenderer().queueRenderBuffer(&m_CircleRenderBuffer);
	}

	m_bNeedResetPos = false;
	m_bNeedResetColor = false;
}

LRESULT GizmoToolController::MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PROFILER_SCOPED_FUNCTION();

	if (uMsg == WM_LBUTTONDOWN || uMsg == WM_MOUSEMOVE || uMsg == WM_LBUTTONUP)
	{
		float mouseX = (short)LOWORD(lParam) / getRenderer().getScreenWidth();
		if (!SimpleScene::Inst()->m_pCamera->_lhs)
		{
			mouseX = 1 - mouseX;
		}
		float mouseY = (short)HIWORD(lParam) / getRenderer().getScreenHeight();

		PxVec3 eyePos, pickDir;
		getPhysXController().getEyePoseAndPickDir(mouseX, mouseY, eyePos, pickDir);
		pickDir = pickDir.getNormalized();

		if (uMsg == WM_LBUTTONDOWN)
		{
			if (m_AxisSelected == AT_Num)
			{
				PxRaycastBufferN<32> hits;
				GetPhysXScene().raycast(eyePos, pickDir, PX_MAX_F32, hits, PxHitFlag::ePOSITION | PxHitFlag::eMESH_MULTIPLE);

				PxU32 nbThouches = hits.getNbTouches();
				const PxRaycastHit* touches = hits.getTouches();

				PxRigidActor* actor = NULL;
				for (PxU32 u = 0; u < nbThouches; ++u)
				{
					const PxRaycastHit& t = touches[u];
					if (t.shape && getBlastController().isActorVisible(*(t.actor)))
					{
						actor = t.actor;
					}
				}

				if (actor)
				{
					PxRigidDynamic* rigidDynamic = actor->is<PxRigidDynamic>();
					if (NULL != rigidDynamic)
					{
						m_CurrentActor = rigidDynamic;
						getSelectionToolController().pointSelect(m_CurrentActor);

						PxTransform gp = m_CurrentActor->getGlobalPose();

						m_TargetPos = gp.p;
						m_Axis[AT_X] = gp.q.rotate(PxVec3(defaultAxisLength, 0, 0));
						m_Axis[AT_Y] = gp.q.rotate(PxVec3(0, defaultAxisLength, 0));
						m_Axis[AT_Z] = gp.q.rotate(PxVec3(0, 0, defaultAxisLength));

						m_bNeedResetPos = true;
					}
					else
					{
						m_CurrentActor = NULL;
						getSelectionToolController().clearSelect();
					}
				}
			}
			else
			{
				m_bGizmoFollowed = (m_CurrentActor != NULL);

				if (m_GizmoToolMode == GTM_Scale)
				{
					m_LastAxis[AT_X] = m_Axis[AT_X].getNormalized();
					m_LastAxis[AT_Y] = m_Axis[AT_Y].getNormalized();
					m_LastAxis[AT_Z] = m_Axis[AT_Z].getNormalized();
				}
			}
		}
		else if (uMsg == WM_MOUSEMOVE)
		{
			if (m_bGizmoFollowed)
			{
				switch (m_GizmoToolMode)
				{
				case GTM_Translate:
					{
						if (AppMainWindow::Inst().m_bGizmoWithLocal && !CanModifyLocal(m_CurrentActor))
						{
							char message[1024];
							sprintf(message, "Only unfractured model can be modify in local way in edit mode!");
							viewer_warn(message);
							return 1;
						}

						PxVec3 axis = m_Axis[m_AxisSelected];
						axis = axis.getNormalized();
						PxVec3 samplepoint = eyePos + pickDir;
						PxVec3 normal = m_LastEyeRay.cross(axis);
						normal = normal.getNormalized();
						PxVec3 foot;
						GetFootFromPointToPlane(samplepoint, eyePos, normal, foot);
						PxVec3 direction = foot - eyePos;
						direction = direction.getNormalized();
						PxVec3 target;
						GetIntersectBetweenLines(m_LastFoot, axis, eyePos, direction, target);
						PxVec3 delta = target - m_LastFoot;

						m_LastEyeRay = direction;
						m_LastFoot = target;

						PxTransform gp_old = m_CurrentActor->getGlobalPose();
						PxTransform gp_new(gp_old.p + delta, gp_old.q);
						m_CurrentActor->setGlobalPose(gp_new);

						bool modifyLocal = AppMainWindow::Inst().m_bGizmoWithLocal && CanModifyLocal(m_CurrentActor);

						if (!SampleManager::ins()->IsSimulating())
						{
							getBlastController().updateActorRenderableTransform(*m_CurrentActor, gp_new, modifyLocal);
							if (CanMapToRootChunk(m_CurrentActor) && !modifyLocal)
								UpdateAssetInstanceTransform(gp_new);
						}

						m_TargetPos = gp_new.p;

						if (modifyLocal)
						{
							modifyPxActorByLocalWay(GetPhysXScene(), *m_CurrentActor, gp_old, gp_new);
						}

						m_bNeedResetPos = true;
						m_bNeedResetColor = true;
					}
					break;
				case GTM_Scale:
					{
						if (AppMainWindow::Inst().m_bGizmoWithLocal && !CanModifyLocal(m_CurrentActor))
						{
							char message[1024];
							sprintf(message, "Only unfractured model can be modify in local way in edit mode!");
							viewer_warn(message);
							return 1;
						}

						PxVec3 axis = m_LastAxis[m_AxisSelected];
						PxVec3 samplepoint = eyePos + pickDir;
						PxVec3 normal = m_LastEyeRay.cross(axis);
						normal = normal.getNormalized();
						PxVec3 foot;
						GetFootFromPointToPlane(samplepoint, eyePos, normal, foot);
						PxVec3 direction = foot - eyePos;
						direction = direction.getNormalized();
						PxVec3 target;
						GetIntersectBetweenLines(m_LastFoot, axis, eyePos, direction, target);
						PxVec3 delta = target - m_LastFoot;

						if (m_AxisSelected == AT_X)
						{
							delta *= defaultAxisModifier;
						}
						m_Axis[m_AxisSelected] = m_LastAxis[m_AxisSelected] * defaultAxisLength + delta;

						bool isShift = (GetAsyncKeyState(VK_SHIFT) && 0x8000);
						if (isShift)
						{
							float length = m_Axis[m_AxisSelected].magnitude();
							m_Axis[AT_X] = m_LastAxis[AT_X] * length;
							m_Axis[AT_Y] = m_LastAxis[AT_Y] * length;
							m_Axis[AT_Z] = m_LastAxis[AT_Z] * length;
						}

						ScaleActor(false);

						m_bNeedResetPos = true;
						m_bNeedResetColor = true;
					}
					break;

				case GTM_Rotation:
					{
						if (AppMainWindow::Inst().m_bGizmoWithLocal && !CanModifyLocal(m_CurrentActor))
						{
							char message[1024];
							sprintf(message, "Only unfractured model can be modify in local way in edit mode!");
							viewer_warn(message);
							return 1;
						}

						PxVec3 planenormal = m_Axis[m_AxisSelected];
						planenormal = planenormal.getNormalized();

						PxVec3 from, to;
						CalPlaneLineIntersectPoint(from, planenormal, m_TargetPos, m_LastEyeRay, eyePos);
						CalPlaneLineIntersectPoint(to, planenormal, m_TargetPos, pickDir, eyePos);
						from = from - m_TargetPos;
						to = to - m_TargetPos;
						from = from.getNormalized();
						to = to.getNormalized();
						float cosangle = from.dot(to);
						float angle = PxAcos(cosangle);
						PxVec3 cross = from.cross(to);
						cross = cross.getNormalized();

						PxQuat q(angle, cross);
						if (m_AxisSelected == AT_X)
						{
							m_Axis[AT_Y] = q.rotate(m_Axis[AT_Y]);
							m_Axis[AT_Z] = q.rotate(m_Axis[AT_Z]);
						}
						else if (m_AxisSelected == AT_Y)
						{
							m_Axis[AT_X] = q.rotate(m_Axis[AT_X]);
							m_Axis[AT_Z] = q.rotate(m_Axis[AT_Z]);
						}
						else if (m_AxisSelected == AT_Z)
						{
							m_Axis[AT_X] = q.rotate(m_Axis[AT_X]);
							m_Axis[AT_Y] = q.rotate(m_Axis[AT_Y]);
						}

						m_LastEyeRay = pickDir;

						PxTransform gp_old = m_CurrentActor->getGlobalPose();
						PxTransform gp_new = PxTransform(gp_old.p, CalConvertQuat());
						m_CurrentActor->setGlobalPose(gp_new);
						bool modifyLocal = AppMainWindow::Inst().m_bGizmoWithLocal && CanModifyLocal(m_CurrentActor);

						if (!SampleManager::ins()->IsSimulating())
						{
							getBlastController().updateActorRenderableTransform(*m_CurrentActor, gp_new, modifyLocal);
							if (CanMapToRootChunk(m_CurrentActor))
								UpdateAssetInstanceTransform(gp_new);
						}

						if (modifyLocal)
						{
							modifyPxActorByLocalWay(GetPhysXScene(), *m_CurrentActor, gp_old, gp_new);
						}

						m_bNeedResetPos = true;
						m_bNeedResetColor = true;
					}
					break;
				}
			}
			else if(m_CurrentActor != NULL)
			{
				m_LastEyeRay = pickDir;

				// get axis which intersect with this eye ray
				AxisType as = AT_Num;
				{
					double distanceMin = PX_MAX_F32;
					double tolerance = defaultAxisLength / 20.0f;
					int line_index = -1;
					PxVec3 foot;
					switch (m_GizmoToolMode)
					{
					case GTM_Translate:
						{
							std::vector<PxDebugLine>& lines = m_AxisRenderBuffer.m_lines;
							int linesize = lines.size();
							for (int l = 0; l < linesize; l++)
							{
								PxVec3 start = lines[l].pos0;
								PxVec3 end = lines[l].pos1;
								PxVec3 dir = end - start;

								// separate the line to 10 segment
								for (int segment = 0; segment <= 10; segment++)
								{
									PxVec3 vertex = start + 0.1 * segment * dir;
									double distance = DistanceFromPointToLine(vertex, eyePos, pickDir, foot);

									if (distance < distanceMin)
									{
										distanceMin = distance;
										line_index = l;
										m_LastFoot = foot;
									}
								}
							}
							if (distanceMin < tolerance)
							{
								int axis_index = line_index * 3 / linesize;
								as = (AxisType)axis_index;
							}
						}
						break;
					case GTM_Scale:
						{
							std::vector<PxDebugLine>& lines = m_AxisRenderBuffer.m_lines;
							int linesize = lines.size();
							for (int l = 0; l < linesize; l++)
							{
								PxVec3 vertex = lines[l].pos1;
								double distance = DistanceFromPointToLine(vertex, eyePos, pickDir, foot);

								if (distance < distanceMin)
								{
									distanceMin = distance;
									line_index = l;
									m_LastFoot = foot;
								}
							}
							if (distanceMin < tolerance)
							{
								as = (AxisType)line_index;
							}
						}
						break;
					case GTM_Rotation:
						{
							std::vector<PxDebugLine>& lines = m_CircleRenderBuffer.m_lines;
							int linesize = lines.size();
							for (int l = 0; l < linesize; l++)
							{
								PxVec3 vertex = lines[l].pos0;
								double distance = DistanceFromPointToLine(vertex, eyePos, pickDir, foot);

								if (distance < distanceMin)
								{
									distanceMin = distance;
									line_index = l;
									m_LastFoot = foot;
								}
							}
							if (distanceMin < tolerance)
							{
								int axis_index = line_index * 3 / linesize;
								as = (AxisType)axis_index;
							}
						}
						break;
					default:
						break;
					}
				}
				setAxisSelected(as);
			}
		}
		else if (uMsg == WM_LBUTTONUP)
		{
			if (m_GizmoToolMode == GTM_Scale)
			{
				if (m_AxisSelected != AT_Num)
				{
					if (NULL != m_CurrentActor)
					{
						ScaleActor(true);
					}

					m_Axis[AT_X] = m_LastAxis[AT_X] * defaultAxisLength;
					m_Axis[AT_Y] = m_LastAxis[AT_Y] * defaultAxisLength;
					m_Axis[AT_Z] = m_LastAxis[AT_Z] * defaultAxisLength;
				}
			}

			if (m_AxisSelected != AT_Num && AppMainWindow::Inst().m_bGizmoWithLocal)
			{
				getBlastController().updateModelMeshToProjectParam(*m_CurrentActor);
			}

			m_bNeedResetPos = true;
			m_bNeedResetColor = true;
			m_bGizmoFollowed = false;
		}
	}

	return 1;
}

void GizmoToolController::drawUI()
{
}

void GizmoToolController::setGizmoToolMode(GizmoToolMode mode)
{
	if (mode == m_GizmoToolMode)
	{
		return;
	}

	m_GizmoToolMode = mode;

	m_bNeedResetPos = true;
	m_bNeedResetColor = true;

	showAxisRenderables(true);
}

void GizmoToolController::setAxisSelected(AxisType type)
{
	if (type == m_AxisSelected)
	{
		return;
	}

	m_AxisSelected = type;
	m_bNeedResetColor = true;
}

void GizmoToolController::setAxisLength(float axisLength)
{
	defaultAxisLength = axisLength;
	UpdateCircleRenderData(axisLength);

	float scale = axisLength / 10.f * 0.2f;
	m_AxisConeRenderable[AT_X]->setScale(PxVec3(scale, 2 * scale, scale));
	m_AxisConeRenderable[AT_Y]->setScale(PxVec3(scale, 2 * scale, scale));
	m_AxisConeRenderable[AT_Z]->setScale(PxVec3(scale, 2 * scale, scale));

	m_AxisBoxRenderable[AT_X]->setScale(PxVec3(scale, scale, scale));
	m_AxisBoxRenderable[AT_Y]->setScale(PxVec3(scale, scale, scale));
	m_AxisBoxRenderable[AT_Z]->setScale(PxVec3(scale, scale, scale));

	m_bNeedResetPos = true;
}

void GizmoToolController::showAxisRenderables(bool show)
{
	bool isTranslate = m_GizmoToolMode == GTM_Translate;
	bool isScale = m_GizmoToolMode == GTM_Scale;

	m_AxisConeRenderable[AT_X]->setHidden(!show || !isTranslate);
	m_AxisConeRenderable[AT_Y]->setHidden(!show || !isTranslate);
	m_AxisConeRenderable[AT_Z]->setHidden(!show || !isTranslate);
	m_AxisBoxRenderable[AT_X]->setHidden(!show || !isScale);
	m_AxisBoxRenderable[AT_Y]->setHidden(!show || !isScale);
	m_AxisBoxRenderable[AT_Z]->setHidden(!show || !isScale);
}

void GizmoToolController::resetPos()
{
	m_TargetPos = PxVec3(-100, -100, -100);
	m_bNeedResetPos = true;

	m_AxisSelected = AT_Num;
	m_bNeedResetColor = true;

	m_CurrentActor = NULL;
}

void GizmoToolController::setTargetActor(PxActor* actor)
{
	m_bNeedResetPos = true;
	m_CurrentActor = nullptr;
	if (actor == nullptr)
	{
		return;
	}
	PxRigidDynamic* rigidDynamic = actor->is<PxRigidDynamic>();
	if (rigidDynamic == nullptr)
	{
		return;
	}

	m_CurrentActor = rigidDynamic;
	getSelectionToolController().pointSelect(m_CurrentActor);

	PxTransform gp = m_CurrentActor->getGlobalPose();

	m_TargetPos = gp.p;
	m_Axis[AT_X] = gp.q.rotate(PxVec3(defaultAxisLength, 0, 0));
	m_Axis[AT_Y] = gp.q.rotate(PxVec3(0, defaultAxisLength, 0));
	m_Axis[AT_Z] = gp.q.rotate(PxVec3(0, 0, defaultAxisLength));
}

PxActor* GizmoToolController::getTargetActor()
{
	return m_CurrentActor;
}

physx::PxScene& GizmoToolController::GetPhysXScene()
{
	if (getManager()->IsSimulating())
	{
		return getPhysXController().getPhysXScene();
	}
	else
	{
		return getPhysXController().getEditPhysXScene();
	}
}

void GizmoToolController::UpdateCircleRenderData(float axisLength)
{
	int segment = 36;
	double span = PxTwoPi / segment;
	PxVec3* vertex = new PxVec3[segment];
	m_CircleRenderData.clear();

	for (int i = 0; i < segment; i++)
	{
		vertex[i].x = 0;
		vertex[i].y = axisLength * PxSin(i * span);
		vertex[i].z = axisLength * PxCos(i * span);
	}
	// x
	for (int i = 0; i < segment - 1; i++)
	{
		m_CircleRenderData.push_back(PxDebugLine(vertex[i], vertex[i + 1], X_DIRECTION_COLOR_U));
	}
	m_CircleRenderData.push_back(PxDebugLine(vertex[segment - 1], vertex[0], X_DIRECTION_COLOR_U));

	for (int i = 0; i < segment; i++)
	{
		vertex[i].x = axisLength * PxCos(i * span);
		vertex[i].y = 0;
		vertex[i].z = axisLength * PxSin(i * span);
	}
	// y
	for (int i = 0; i < segment - 1; i++)
	{
		m_CircleRenderData.push_back(PxDebugLine(vertex[i], vertex[i + 1], Y_DIRECTION_COLOR_U));
	}
	m_CircleRenderData.push_back(PxDebugLine(vertex[segment - 1], vertex[0], Y_DIRECTION_COLOR_U));

	for (int i = 0; i < segment; i++)
	{
		vertex[i].x = axisLength * PxCos(i * span);
		vertex[i].y = axisLength * PxSin(i * span);
		vertex[i].z = 0;
	}
	// z
	for (int i = 0; i < segment - 1; i++)
	{
		m_CircleRenderData.push_back(PxDebugLine(vertex[i], vertex[i + 1], Z_DIRECTION_COLOR_U));
	}
	m_CircleRenderData.push_back(PxDebugLine(vertex[segment - 1], vertex[0], Z_DIRECTION_COLOR_U));

	delete[] vertex;
	vertex = NULL;
}

bool GizmoToolController::CalPlaneLineIntersectPoint(PxVec3& result, PxVec3 planeNormal, PxVec3 planePoint, PxVec3 linedirection, PxVec3 linePoint)
{
	float dot = planeNormal.dot(linedirection);
	if (dot == 0)
	{
		return false;
	}
	else
	{
		float t = ((planePoint[0] - linePoint[0]) * planeNormal[0] +
			(planePoint[1] - linePoint[1]) * planeNormal[1] +
			(planePoint[2] - linePoint[2]) * planeNormal[2]) / dot;
		result = linePoint + linedirection * t;
	}
	return true;
}

float GizmoToolController::DistanceFromPointToLine(PxVec3& point, PxVec3& origin, PxVec3& direction, PxVec3& foot)
{
	direction = direction.getNormalized();
	PxVec3 sub = point - origin;
	float t = direction.dot(sub);
	foot = origin + direction * t;
	PxVec3 dis = point - foot;
	return dis.magnitude();
}

bool GizmoToolController::GetFootFromPointToPlane(PxVec3& point, PxVec3& origin, PxVec3& normal, PxVec3& foot)
{
	return CalPlaneLineIntersectPoint(foot, normal, origin, normal, point);
}

bool GizmoToolController::GetIntersectBetweenLines(PxVec3& origin1, PxVec3& direction1, PxVec3& origin2, PxVec3& direction2, PxVec3& intersect)
{
	float test = ((origin2 - origin1).getNormalized()).dot(direction1.cross(direction2));
	if (direction1.cross(direction2).isZero())
	{// if two lines are parallel
		return false;
	}
	else if (abs(test) >= 0.001)
	{// if two lines aren't in the same plane
		return false;
	}

	PxVec3 normal1 = direction1.cross(direction2);
	PxVec3 normal2 = normal1.cross(direction1);
	normal2 = normal2.getNormalized();
	return CalPlaneLineIntersectPoint(intersect, normal2, origin1, direction2, origin2);
}

PxQuat GizmoToolController::CalDirectionQuat(AxisType type)
{
	PxVec3 origin(0, 1, 0);
	PxVec3 target = m_Axis[type];
	if (type == AT_X)
	{
		target *= defaultAxisModifier;
	}
	target = target.getNormalized();
	PxVec3 cross = origin.cross(target);
	cross = cross.getNormalized();
	float cos = origin.dot(target);
	float angle = PxAcos(cos);
	PxQuat q(angle, cross);
	return q;
}

PxQuat GizmoToolController::CalConvertQuat()
{
	PxVec3 x_origin(1, 0, 0);
	PxVec3 y_origin(0, 1, 0);
	PxVec3 z_origin(0, 0, 1);

	PxVec3 x_target = m_Axis[AT_X];
	PxVec3 y_target = m_Axis[AT_Y];
	x_target = x_target.getNormalized();
	y_target = y_target.getNormalized();

	PxVec3 x_cross = x_origin.cross(x_target);
	x_cross = x_cross.getNormalized();
	float x_cos = x_origin.dot(x_target);
	float x_angle = PxAcos(x_cos);
	PxQuat x_quat(x_angle, x_cross);

	PxVec3 y_interval = x_quat.rotate(y_origin);

	PxVec3 y_cross = y_interval.cross(y_target);
	y_cross = y_cross.getNormalized();
	float y_cos = y_interval.dot(y_target);
	float y_angle = PxAcos(y_cos);
	PxQuat y_quat(y_angle, y_cross);

	PxQuat q = y_quat * x_quat;
	return q;
}

void GizmoToolController::ScaleActor(bool replace)
{
	if (nullptr == m_CurrentActor)
	{
		return;
	}

	bool isLocal = AppMainWindow::Inst().m_bGizmoWithLocal;

	if (isLocal && !CanModifyLocal(m_CurrentActor))
	{
		char message[1024];
		sprintf(message, "Only unfractured model can be modify in local way in edit mode!");
		viewer_warn(message);
		return;
	}

	BlastFamilyPtr pBlastFamily = getBlastController().getFamilyByPxActor(*m_CurrentActor);
	if (NULL == pBlastFamily)
	{
		return;
	}

	float multiply = m_Axis[m_AxisSelected].magnitude() / defaultAxisLength;
	if (m_Axis[m_AxisSelected].dot(m_LastAxis[m_AxisSelected]) < 0)
	{
		multiply = -multiply;
	}
	PxVec3 delta(1, 1, 1);
	bool isShift = (GetAsyncKeyState(VK_SHIFT) && 0x8000);
	if (isShift)
	{
		delta *= multiply;
	}
	else
	{
		delta[m_AxisSelected] = multiply;
	}
	PxMat44 scale = PxMat44(PxVec4(delta, 1));

	if (!isLocal)
	{
		PxTransform gp = m_CurrentActor->getGlobalPose();
		uint32_t shapesCount = m_CurrentActor->getNbShapes();
		if (shapesCount > 0)
		{
			std::vector<PxShape*> shapes(shapesCount);
			m_CurrentActor->getShapes(&shapes[0], shapesCount);
			PxShape* shape = shapes[0];
			PxTransform lp = shape->getLocalPose();
			gp = gp * lp;
		}
		PxMat44 world = PxMat44(gp);
		PxMat44 worldInv = world.inverseRT();
		scale = world * scale * worldInv;
	}

	pBlastFamily->setActorScale(*m_CurrentActor, scale, replace);

	if (!replace)
	{
		return;
	}

	scalePxActor(GetPhysXScene(), *m_CurrentActor, scale);
}

bool GizmoToolController::CanMapToRootChunk(PxActor* actor)
{
	if (actor)
	{
		BlastFamily* family = getBlastController().getFamilyByPxActor(*actor);
		if (family == nullptr)
			return false;

		const BlastAsset& asset = family->getBlastAsset();
		uint32_t chunkIndex = family->getChunkIndexByPxActor(*actor);

		std::vector<uint32_t> chunkIndexes;
		chunkIndexes.push_back(chunkIndex);
		std::vector<BlastChunkNode*> chunkNodes = BlastTreeData::ins().getChunkNodeByBlastChunk(&asset, chunkIndexes);
		if (chunkNodes.size() > 0)
			return BlastTreeData::isRoot(chunkNodes[0]);
	}
	return false;
}

bool GizmoToolController::CanModifyLocal(PxActor* actor)
{
	if (nullptr != actor 
		&& !SampleManager::ins()->IsSimulating()
		&& !getBlastController().isAssetFractrued(*actor))
	{
		return true;
	}

	return false;
}

void GizmoToolController::UpdateAssetInstanceTransform(const PxTransform& position)
{
	if (!m_CurrentActor)
		return;

	BlastFamily* family = getBlastController().getFamilyByPxActor(*m_CurrentActor);
	if (family)
	{
		BPPAssetInstance* bppInstance = SampleManager::ins()->getInstanceByFamily(family);

		if (bppInstance)
		{
			bppInstance->transform.position = *((nvidia::NvVec3*)(&position.p));
			bppInstance->transform.rotation = *((nvidia::NvVec4*)(&position.q));

			family->initTransform(position);
			// modify corresponding asset's transform, it's need to modify in future
			{
				const BlastAsset& asset = family->getBlastAsset();
				SampleManager* sampleManager = SampleManager::ins();
				std::map<BlastAsset*, std::vector<BlastFamily*>>& AssetFamiliesMap = sampleManager->getAssetFamiliesMap();
				std::map<BlastAsset*, AssetList::ModelAsset>& AssetDescMap = sampleManager->getAssetDescMap();

				BlastAsset* pBlastAsset = (BlastAsset*)&asset;

				AssetList::ModelAsset& m = AssetDescMap[pBlastAsset];
				m.transform = position;
			}
		}
	}
}

void GizmoToolController::syncRenderableState()
{
	bool depthTest = AppMainWindow::Inst().m_bGizmoWithDepthTest;
	m_AxisConeRenderable[AT_X]->setDepthTest(depthTest);
	m_AxisConeRenderable[AT_Y]->setDepthTest(depthTest);
	m_AxisConeRenderable[AT_Z]->setDepthTest(depthTest);
	m_AxisBoxRenderable[AT_X]->setDepthTest(depthTest);
	m_AxisBoxRenderable[AT_Y]->setDepthTest(depthTest);
	m_AxisBoxRenderable[AT_Z]->setDepthTest(depthTest);
}