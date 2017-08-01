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


#include "SelectionToolController.h"
#include "RenderUtils.h"
#include "BlastController.h"
#include "Renderer.h"
#include "PhysXController.h"
#include "SampleProfiler.h"
#include "GizmoToolController.h"

#include <imgui.h>

#include "NvBlastTkActor.h"
#include "NvBlastExtDamageShaders.h"

#include "PxRigidDynamic.h"
#include "PxScene.h"
#include "BlastSceneTree.h"
#include "SimpleScene.h"
using namespace Nv::Blast;
using namespace physx;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Setup
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


SelectionToolController::SelectionToolController()
{
	m_bRectSelecting = false;
	m_bSelecting = false;

	BlastSceneTree::ins()->addObserver(this);
}

SelectionToolController::~SelectionToolController()
{
}

void SelectionToolController::onSampleStart()
{
}

void SelectionToolController::onInitialize()
{
}


void SelectionToolController::onSampleStop() 
{
}

void SelectionToolController::dataSelected(std::vector<BlastNode*> selections)
{
	m_actorsSelected.clear();

	BlastController& blastController = getBlastController();
	std::vector<BlastFamilyPtr>& families = blastController.getFamilies();

	for (BlastFamily* family : families)
	{
		std::vector<uint32_t> selectedChunks = family->getSelectedChunks();
		for (uint32_t chunkIndex : selectedChunks)
		{
			PxActor* actor = nullptr;
			family->getPxActorByChunkIndex(chunkIndex, &actor);

			if (actor)
			{
				m_actorsSelected.emplace(actor);
			}
		}
	}
}

void SelectionToolController::Animate(double dt)
{
	PROFILER_SCOPED_FUNCTION();

	if (m_bRectSelecting)
	{
		getRenderer().queueRenderBuffer(&m_RectRenderBuffer);
	}
}


LRESULT SelectionToolController::MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PROFILER_SCOPED_FUNCTION();

	if (uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN || uMsg == WM_MOUSEMOVE || uMsg == WM_LBUTTONUP || uMsg == WM_RBUTTONUP)
	{
		float mouseX = (short)LOWORD(lParam) / getRenderer().getScreenWidth();
		if (!SimpleScene::Inst()->m_pCamera->_lhs)
		{
			mouseX = 1 - mouseX;
		}
		float mouseY = (short)HIWORD(lParam) / getRenderer().getScreenHeight();
		bool press = uMsg == WM_LBUTTONDOWN;

		if (uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN)
		{
			PxVec3 eyePos, pickDir;
			getPhysXController().getEyePoseAndPickDir(mouseX, mouseY, eyePos, pickDir);

			m_RectSelectScreenPos.x = mouseX;
			m_RectSelectScreenPos.y = mouseY;
			m_RectSelectSpaceDir = pickDir.getNormalized();

			m_RectRenderBuffer.clear();
			bool isCtrl = (GetAsyncKeyState(VK_CONTROL) && 0x8000);
			bool isAlt = (GetAsyncKeyState(VK_MENU) && 0x8000);
			bool isLight = (GetAsyncKeyState('L') && 0x8000);
			// ctrl+leftbutton is used for light changing
			// alt+leftbutton is used for camera rotate movement in AppMainWindow.cpp
			// so, we use rect select when ctrl and alt off
			m_bRectSelecting = !(isAlt || isLight);// !(isCtrl || isAlt);
			if (isAlt || isLight)
			{
				m_RectRenderBuffer.clear();
			}
			m_bSelecting = true;
		}
		else if (uMsg == WM_MOUSEMOVE)
		{
			if (m_bRectSelecting)
			{
				PxVec3 eyePos, pickDir[3];
				getPhysXController().getEyePoseAndPickDir(mouseX, mouseY, eyePos, pickDir[0]);
				getPhysXController().getEyePoseAndPickDir(m_RectSelectScreenPos.x, mouseY, eyePos, pickDir[1]);
				getPhysXController().getEyePoseAndPickDir(mouseX, m_RectSelectScreenPos.y, eyePos, pickDir[2]);
				pickDir[0] = pickDir[0].getNormalized();
				pickDir[1] = pickDir[1].getNormalized();
				pickDir[2] = pickDir[2].getNormalized();

				PxVec3 lefttop, leftbottom, righttop, rightbottom;

				if (mouseX > m_RectSelectScreenPos.x) // right
				{
					if (mouseY > m_RectSelectScreenPos.y) // top
					{
						leftbottom = m_RectSelectSpaceDir;
						righttop = pickDir[0];
						lefttop = pickDir[2];
						rightbottom = pickDir[1];
					}
					else // bottom
					{
						leftbottom = pickDir[1];
						righttop = pickDir[2];
						lefttop = m_RectSelectSpaceDir;
						rightbottom = pickDir[0];
					}
				}
				else // left
				{
					if (mouseY > m_RectSelectScreenPos.y) // top
					{
						leftbottom = pickDir[2];
						righttop = pickDir[1];
						lefttop = pickDir[0];
						rightbottom = m_RectSelectSpaceDir;
					}
					else // bottom
					{
						leftbottom = pickDir[0];
						righttop = m_RectSelectSpaceDir;
						lefttop = pickDir[2];
						rightbottom = pickDir[1];
					}
				}

				float multiply = 10; // in order to draw line
				lefttop = eyePos + lefttop * multiply;
				righttop = eyePos + righttop * multiply;
				leftbottom = eyePos + leftbottom * multiply;
				rightbottom = eyePos + rightbottom * multiply;

				m_RectRenderBuffer.clear();

				DirectX::XMFLOAT4 LINE_COLOR(1.0f, 0.0f, 0.0f, 1.0f);
				m_RectRenderBuffer.m_lines.push_back(PxDebugLine(lefttop, leftbottom, XMFLOAT4ToU32Color(LINE_COLOR)));
				m_RectRenderBuffer.m_lines.push_back(PxDebugLine(lefttop, righttop, XMFLOAT4ToU32Color(LINE_COLOR)));
				m_RectRenderBuffer.m_lines.push_back(PxDebugLine(righttop, rightbottom, XMFLOAT4ToU32Color(LINE_COLOR)));
				m_RectRenderBuffer.m_lines.push_back(PxDebugLine(leftbottom, rightbottom, XMFLOAT4ToU32Color(LINE_COLOR)));
			}
		}
		else if (uMsg == WM_LBUTTONUP || uMsg == WM_RBUTTONUP)
		{
			if (uMsg == WM_RBUTTONUP && m_actorsSelected.size() > 1)
			{
				return 1;
			}
			bool isAlt = (GetAsyncKeyState(VK_MENU) && 0x8000);
			bool isLight = (GetAsyncKeyState('L') && 0x8000);
			if (m_bSelecting && !(isAlt || isLight))
			{
				bool isShift = (GetAsyncKeyState(VK_SHIFT) && 0x8000);
				bool isCtrl = (GetAsyncKeyState(VK_CONTROL) && 0x8000);

				SelectMode selectMode = SM_RESET;
				if (isShift && isCtrl)
				{
					selectMode = SM_REMAIN;
				}
				else if (isShift)
				{
					selectMode = SM_ADD;
				}
				else if (isCtrl)
				{
					selectMode = SM_SUB;
				}
				int width = getRenderer().getScreenWidth();
				int height = getRenderer().getScreenHeight();
				int deltaX = (mouseX - m_RectSelectScreenPos.x) * width;
				int deltaY = (mouseY - m_RectSelectScreenPos.y) * height;
				float distance = deltaX * deltaX + deltaY * deltaY;
				if (distance < 1)
				{
					m_bRectSelecting = false;
				}
				if (m_bRectSelecting)
				{
					// rect select mode
					PxVec3 eyePos, pickDir[3];
					getPhysXController().getEyePoseAndPickDir(mouseX, mouseY, eyePos, pickDir[0]);
					getPhysXController().getEyePoseAndPickDir(m_RectSelectScreenPos.x, mouseY, eyePos, pickDir[1]);
					getPhysXController().getEyePoseAndPickDir(mouseX, m_RectSelectScreenPos.y, eyePos, pickDir[2]);
					pickDir[0] = pickDir[0].getNormalized();
					pickDir[1] = pickDir[1].getNormalized();
					pickDir[2] = pickDir[2].getNormalized();

					PxVec3 lefttop, leftbottom, righttop, rightbottom;

					if (mouseX > m_RectSelectScreenPos.x) // right
					{
						if (mouseY > m_RectSelectScreenPos.y) // top
						{
							leftbottom = m_RectSelectSpaceDir;
							righttop = pickDir[0];
							lefttop = pickDir[2];
							rightbottom = pickDir[1];
						}
						else // bottom
						{
							leftbottom = pickDir[1];
							righttop = pickDir[2];
							lefttop = m_RectSelectSpaceDir;
							rightbottom = pickDir[0];
						}
					}
					else // left
					{
						if (mouseY > m_RectSelectScreenPos.y) // top
						{
							leftbottom = pickDir[2];
							righttop = pickDir[1];
							lefttop = pickDir[0];
							rightbottom = m_RectSelectSpaceDir;
						}
						else // bottom
						{
							leftbottom = pickDir[0];
							righttop = m_RectSelectSpaceDir;
							lefttop = pickDir[2];
							rightbottom = pickDir[1];
						}
					}

					rectSelect(eyePos, lefttop, leftbottom, righttop, rightbottom, selectMode);
				}
				// point select mode
				else
				{
					PxVec3 eyePos, pickDir;
					getPhysXController().getEyePoseAndPickDir(mouseX, mouseY, eyePos, pickDir);
					pickDir = pickDir.getNormalized();

					PxRaycastBufferN<32> hits;

					GetPhysXScene().raycast(eyePos, pickDir, PX_MAX_F32, hits, PxHitFlag::eDEFAULT | PxHitFlag::eMESH_MULTIPLE);

					PxU32 nbThouches = hits.getNbTouches();
					const PxRaycastHit* touches = hits.getTouches();

					PxRigidActor* actor = NULL;
					float fDistance = PX_MAX_F32;
					for (PxU32 u = 0; u < nbThouches; ++u)
					{
						const PxRaycastHit& t = touches[u];
						if (t.shape && getBlastController().isActorVisible(*(t.actor)))
						{
							if (fDistance > t.distance)
							{
								fDistance = t.distance;
								actor = t.actor;
							}
						}
					}

					pointSelect(actor, selectMode);
				}
			}

			m_RectRenderBuffer.clear();
			m_bRectSelecting = false;
			m_bSelecting = false;
		}
	}

	return 1;
}

void SelectionToolController::drawUI()
{
}

void SelectionToolController::pointSelect(PxActor* actor, SelectMode selectMode)
{
	if (selectMode == SM_RESET)
	{
		clearSelect();

		if (NULL != actor)
		{
			setActorSelected(*actor, true);
			m_actorsSelected.emplace(actor);
		}
	}
	else if (selectMode == SM_ADD)
	{
		if (NULL != actor)
		{
			setActorSelected(*actor, true);
			m_actorsSelected.emplace(actor);
		}
	}
	else if (selectMode == SM_SUB)
	{
		if (NULL != actor)
		{
			setActorSelected(*actor, false);
			m_actorsSelected.erase(actor);
		}
	}

	BlastSceneTree::ins()->updateChunkItemSelection();
	trySelectAssetInstanceNode(m_actorsSelected);
}

#include "PxPhysics.h"
#include "cooking/PxCooking.h"

class RectSelectionCallback : public PxOverlapCallback
{
public:
	RectSelectionCallback(std::set<PxActor*>& actorBuffer)
		:m_actorBuffer(actorBuffer), PxOverlapCallback(m_hitBuffer, sizeof(m_hitBuffer) / sizeof(m_hitBuffer[0])) {}

	PxAgain processTouches(const PxOverlapHit* buffer, PxU32 nbHits)
	{
		for (PxU32 i = 0; i < nbHits; ++i)
		{
			PxRigidDynamic* rigidDynamic = buffer[i].actor->is<PxRigidDynamic>();
			if (rigidDynamic)
			{
				m_actorBuffer.insert(rigidDynamic);
			}
		}
		return true;
	}

private:
	std::set<PxActor*>&				m_actorBuffer;
	PxOverlapHit					m_hitBuffer[1000];
};

void SelectionToolController::rectSelect(PxVec3 eyePos, PxVec3 lefttop, PxVec3 leftbottom, PxVec3 righttop, PxVec3 rightbottom, SelectMode selectMode)
{
	std::set<PxActor*> actorsToSelect;

	float nearClip = 1;
	PxVec3 nearlefttop = lefttop * nearClip;
	PxVec3 nearrighttop = righttop * nearClip;
	PxVec3 nearleftbottom = leftbottom * nearClip;
	PxVec3 nearrightbottom = rightbottom * nearClip;

	float farClip = 1000;
	PxVec3 farlefttop = lefttop * farClip;
	PxVec3 farrighttop = righttop * farClip;
	PxVec3 farleftbottom = leftbottom * farClip;
	PxVec3 farrightbottom = rightbottom * farClip;

	PxVec3 vertices[8] =
	{
		nearlefttop, nearrighttop, nearleftbottom, nearrightbottom,
		farlefttop, farrighttop, farleftbottom, farrightbottom
	};
	PxConvexMeshDesc convexMeshDesc;
	convexMeshDesc.points.count = 8;
	convexMeshDesc.points.data = vertices;
	convexMeshDesc.points.stride = sizeof(PxVec3);
	convexMeshDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;	
	PxPhysics& physics = getManager()->getPhysXController().getPhysics();
	PxCooking& cooking = getManager()->getPhysXController().getCooking();
	PxConvexMesh* convexMesh = cooking.createConvexMesh(convexMeshDesc, physics.getPhysicsInsertionCallback());
	if (NULL != convexMesh)
	{
		RectSelectionCallback overlapCallback(actorsToSelect);
		GetPhysXScene().overlap(PxConvexMeshGeometry(convexMesh), PxTransform(eyePos), overlapCallback);
		convexMesh->release();
	}

	if (selectMode == SM_RESET)
	{
		clearSelect();

		for (PxActor* actor : actorsToSelect)
		{
			if (getBlastController().isActorVisible(*actor))
			{
				setActorSelected(*actor, true);
				m_actorsSelected.emplace(actor);
			}
		}
	}
	else if (selectMode == SM_ADD)
	{
		for (PxActor* actor : actorsToSelect)
		{
			if (getBlastController().isActorVisible(*actor))
			{
				setActorSelected(*actor, true);
				m_actorsSelected.emplace(actor);
			}
		}
	}
	else if (selectMode == SM_SUB)
	{
		for (PxActor* actor : actorsToSelect)
		{
			setActorSelected(*actor, false);
			m_actorsSelected.erase(actor);
		}
	}

	BlastSceneTree::ins()->updateChunkItemSelection();
	trySelectAssetInstanceNode(m_actorsSelected);
}

void SelectionToolController::clearSelect()
{
	for (PxActor* actor : m_actorsSelected)
	{
		setActorSelected(*actor, false);
	}

	m_actorsSelected.clear();
	SampleManager::ins()->clearChunksSelected();
}

void SelectionToolController::setTargetActor(PxActor* actor)
{
	if (actor == nullptr)
	{
		return;
	}

	setActorSelected(*actor, true);
	m_actorsSelected.emplace(actor);

	BlastSceneTree::ins()->updateChunkItemSelection();
	trySelectAssetInstanceNode(m_actorsSelected);
}

PxActor* SelectionToolController::getTargetActor()
{
	PxActor* targetActor = nullptr;
	if (m_actorsSelected.size() > 0)
	{
		targetActor = *m_actorsSelected.begin();
	}
	return targetActor;
}

void SelectionToolController::setTargetActors(std::set<PxActor*>& actors)
{
	for (PxActor* actor : actors)
	{
		setActorSelected(*actor, true);
		m_actorsSelected.emplace(actor);
	}

	BlastSceneTree::ins()->updateChunkItemSelection();
	trySelectAssetInstanceNode(m_actorsSelected);
}

std::set<PxActor*> SelectionToolController::getTargetActors()
{
	return m_actorsSelected;
}

void SelectionToolController::trySelectAssetInstanceNode(std::set<PxActor*>& selectedActors)
{
	BlastController& blastController = getBlastController();
	GizmoToolController& gizmoToolController = getManager()->getGizmoToolController();

	for (PxActor* actor : selectedActors)
	{
		if (gizmoToolController.CanMapToRootChunk(actor))
		{
			BlastFamily* family = getBlastController().getFamilyByPxActor(*actor);
			BlastSceneTree::ins()->selectTreeItem(family);
		}
	}
}

physx::PxScene& SelectionToolController::GetPhysXScene()
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

void SelectionToolController::setActorSelected(const PxActor& actor, bool selected)
{
	std::vector<BlastFamilyPtr>& families = getBlastController().getFamilies();
	if (families.size() == 0)
	{
		return;
	}

	BlastFamilyPtr pBlastFamily = NULL;
	std::vector<BlastFamilyPtr>::iterator it = families.begin();
	for (; it != families.end(); it++)
	{
		BlastFamilyPtr f = *it;
		if (f->find(actor))
		{
			pBlastFamily = f;
			break;
		}
	}
	if (NULL == pBlastFamily)
	{
		return;
	}

	pBlastFamily->setActorSelected(actor, selected);
}