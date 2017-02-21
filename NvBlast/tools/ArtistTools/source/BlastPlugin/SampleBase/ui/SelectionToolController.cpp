/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "SelectionToolController.h"
#include "RenderUtils.h"
#include "BlastController.h"
#include "Renderer.h"
#include "PhysXController.h"
#include "SampleProfiler.h"

#include <imgui.h>

#include "NvBlastTkActor.h"
#include "NvBlastExtDamageShaders.h"

#include "PxRigidDynamic.h"
#include "PxScene.h"
#include "BlastSceneTree.h"

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
			m_bRectSelecting = true;
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

			if (m_bRectSelecting)
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
				// rect select mode
				if (distance > 1)
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

					rectSelect(eyePos, lefttop, leftbottom, righttop, rightbottom, selectMode);
				}
				// point select mode
				else
				{
					PxVec3 eyePos, pickDir;
					getPhysXController().getEyePoseAndPickDir(mouseX, mouseY, eyePos, pickDir);
					pickDir = pickDir.getNormalized();

					PxRaycastHit hit; hit.shape = NULL;
					PxRaycastBuffer hit1;
					getPhysXController().getPhysXScene().raycast(eyePos, pickDir, PX_MAX_F32, hit1, PxHitFlag::ePOSITION | PxHitFlag::eNORMAL);
					hit = hit1.block;

					PxRigidActor* actor = NULL;
					if (hit.shape)
					{
						actor = hit.actor;
					}
					pointSelect(actor, selectMode);
				}
			}

			BlastSceneTree::ins()->updateChunkItemSelection();
			m_RectRenderBuffer.clear();
			m_bRectSelecting = false;
		}
	}

	return 1;
}

void SelectionToolController::drawUI()
{
}

void SelectionToolController::pointSelect(PxActor* actor, SelectMode selectMode)
{
	ExtPxActor* extActor = NULL;
	if (NULL != actor)
	{
		PxRigidDynamic* rigidDynamic = actor->is<PxRigidDynamic>();
		if (NULL != rigidDynamic)
		{
			extActor = getBlastController().getExtPxManager().getActorFromPhysXActor(*rigidDynamic);
		}
	}

	if (selectMode == SM_RESET)
	{
		clearSelect();

		if (NULL != extActor)
		{
			setActorSelected(*extActor, true);
			m_actorsSelected.emplace(extActor);
		}
	}
	else if (selectMode == SM_ADD)
	{
		if (NULL != extActor)
		{
			setActorSelected(*extActor, true);
			m_actorsSelected.emplace(extActor);
		}
	}
	else if (selectMode == SM_SUB)
	{
		if (NULL != extActor)
		{
			setActorSelected(*extActor, false);
			m_actorsSelected.erase(extActor);
		}
	}
}

#include "PxPhysics.h"
#include "cooking/PxCooking.h"

class RectSelectionCallback : public PxOverlapCallback
{
public:
	RectSelectionCallback(ExtPxManager& physicsManager, std::set<ExtPxActor*>& actorBuffer)
		: m_physicsManager(physicsManager), m_actorBuffer(actorBuffer), PxOverlapCallback(m_hitBuffer, sizeof(m_hitBuffer) / sizeof(m_hitBuffer[0])) {}

	PxAgain processTouches(const PxOverlapHit* buffer, PxU32 nbHits)
	{
		for (PxU32 i = 0; i < nbHits; ++i)
		{
			PxRigidDynamic* rigidDynamic = buffer[i].actor->is<PxRigidDynamic>();
			if (rigidDynamic)
			{
				ExtPxActor* actor = m_physicsManager.getActorFromPhysXActor(*rigidDynamic);
				if (actor != nullptr)
				{
					m_actorBuffer.insert(actor);
				}
			}
		}
		return true;
	}

private:
	ExtPxManager&						m_physicsManager;
	std::set<ExtPxActor*>&				m_actorBuffer;
	PxOverlapHit						m_hitBuffer[1000];
};

void SelectionToolController::rectSelect(PxVec3 eyePos, PxVec3 lefttop, PxVec3 leftbottom, PxVec3 righttop, PxVec3 rightbottom, SelectMode selectMode)
{
	std::set<ExtPxActor*> actorsToSelect;

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
		RectSelectionCallback overlapCallback(getBlastController().getExtPxManager(), actorsToSelect);
		getManager()->getPhysXController().getPhysXScene().overlap(PxConvexMeshGeometry(convexMesh), PxTransform(eyePos), overlapCallback);
		convexMesh->release();
	}

	if (selectMode == SM_RESET)
	{
		clearSelect();

		for (ExtPxActor* actor : actorsToSelect)
		{
			setActorSelected(*actor, true);
			m_actorsSelected.emplace(actor);
		}
	}
	else if (selectMode == SM_ADD)
	{
		for (ExtPxActor* actor : actorsToSelect)
		{
			setActorSelected(*actor, true);
			m_actorsSelected.emplace(actor);
		}
	}
	else if (selectMode == SM_SUB)
	{
		for (ExtPxActor* actor : actorsToSelect)
		{
			setActorSelected(*actor, false);
			m_actorsSelected.erase(actor);
		}
	}
}

void SelectionToolController::clearSelect()
{
	for (ExtPxActor* actor : m_actorsSelected)
	{
		setActorSelected(*actor, false);
	}

	m_actorsSelected.clear();
	SampleManager::ins()->clearChunksSelected();
}

void SelectionToolController::setActorSelected(const ExtPxActor& actor, bool selected)
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
		if (f->find((ExtPxActor*)&actor))
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