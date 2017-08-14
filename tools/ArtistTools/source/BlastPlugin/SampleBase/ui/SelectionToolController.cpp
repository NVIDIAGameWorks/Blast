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
	m_bDrawSelecting = false;
	m_bSelecting = false;
	m_ScreenRenderBuffer.clear();
	m_DrawSelectScreenPos.clear();

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

	if (m_ScreenRenderBuffer.getNbLines() > 0)
	{		
		getRenderer().queueRenderBuffer(&m_ScreenRenderBuffer, true);
	}
}


LRESULT SelectionToolController::MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PROFILER_SCOPED_FUNCTION();

	if (uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN || uMsg == WM_MOUSEMOVE || uMsg == WM_LBUTTONUP || uMsg == WM_RBUTTONUP)
	{
		float mouseX = (short)LOWORD(lParam) / getRenderer().getScreenWidth();
		float mouseY = (short)HIWORD(lParam) / getRenderer().getScreenHeight();

		if (uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN)
		{
			m_ScreenRenderBuffer.clear();

			bool isCtrl = (GetAsyncKeyState(VK_CONTROL) && 0x8000);
			bool isAlt = (GetAsyncKeyState(VK_MENU) && 0x8000);
			bool isLight = (GetAsyncKeyState('L') && 0x8000);
			// ctrl+leftbutton is used for light changing
			// alt+leftbutton is used for camera rotate movement in AppMainWindow.cpp
			// so, we use rect select when ctrl and alt off
			bool Selecting = !(isAlt || isLight);// !(isCtrl || isAlt);
			
			if (uMsg == WM_LBUTTONDOWN && !m_bDrawSelecting)
			{
				m_RectSelectScreenPos.x = mouseX;
				m_RectSelectScreenPos.y = mouseY;

				m_bRectSelecting = Selecting;
			}
			else // uMsg == WM_RBUTTONDOWN
			{
				m_DrawSelectScreenPos.push_back(PxVec2(mouseX, mouseY));

				m_bDrawSelecting = Selecting;
			}

			m_bSelecting = m_bRectSelecting || m_bDrawSelecting;
		}
		else if (uMsg == WM_MOUSEMOVE)
		{
			if (m_bRectSelecting)
			{
				float left, right, top, bottom;
				left = right = mouseX;
				top = bottom = mouseY;
				if (mouseX > m_RectSelectScreenPos.x)
				{
					left = m_RectSelectScreenPos.x;
				}
				else
				{
					right = m_RectSelectScreenPos.x;
				}
				if (mouseY > m_RectSelectScreenPos.y)
				{
					top = m_RectSelectScreenPos.y;
				}
				else
				{
					bottom = m_RectSelectScreenPos.y;
				}

				m_ScreenRenderBuffer.clear();

				PxVec3 left_top(left, top, 0);
				PxVec3 left_bottom(left, bottom, 0);
				PxVec3 right_bottom(right, bottom, 0);
				PxVec3 right_top(right, top, 0);
				DirectX::XMFLOAT4 LINE_COLOR(1.0f, 0.0f, 0.0f, 1.0f);
				m_ScreenRenderBuffer.m_lines.push_back(PxDebugLine(left_top, left_bottom, XMFLOAT4ToU32Color(LINE_COLOR)));
				m_ScreenRenderBuffer.m_lines.push_back(PxDebugLine(left_bottom, right_bottom, XMFLOAT4ToU32Color(LINE_COLOR)));
				m_ScreenRenderBuffer.m_lines.push_back(PxDebugLine(right_bottom, right_top, XMFLOAT4ToU32Color(LINE_COLOR)));
				m_ScreenRenderBuffer.m_lines.push_back(PxDebugLine(right_top, left_top, XMFLOAT4ToU32Color(LINE_COLOR)));
			}
		}
		else if (uMsg == WM_LBUTTONUP)
		{
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
				
				Renderer* pRenderer = Renderer::Inst();
				std::vector<PxVec2> screenPoints;
				std::map<int, std::set<int>> selection;

				if (m_bRectSelecting)
				{
					int width = getRenderer().getScreenWidth();
					int height = getRenderer().getScreenHeight();
					int deltaX = (mouseX - m_RectSelectScreenPos.x) * width;
					int deltaY = (mouseY - m_RectSelectScreenPos.y) * height;
					float distance = deltaX * deltaX + deltaY * deltaY;
					if (distance < 1)
					{
						// point select
						screenPoints.push_back(m_RectSelectScreenPos);
					}
					else
					{
						// rect select	
						float left, right, top, bottom;
						left = right = mouseX;
						top = bottom = mouseY;
						if (mouseX > m_RectSelectScreenPos.x)
						{
							left = m_RectSelectScreenPos.x;
						}
						else
						{
							right = m_RectSelectScreenPos.x;
						}
						if (mouseY > m_RectSelectScreenPos.y)
						{
							top = m_RectSelectScreenPos.y;
						}
						else
						{
							bottom = m_RectSelectScreenPos.y;
						}

						screenPoints.push_back(PxVec2(left, top));
						screenPoints.push_back(PxVec2(right, bottom));
					}
				}
				else if(m_bDrawSelecting)
				{
					// draw select
					if (m_DrawSelectScreenPos.size() > 2)
					{
						screenPoints.swap(m_DrawSelectScreenPos);
					}
				}

				pRenderer->fetchSelection(screenPoints, selection);

				std::set<PxActor*> actors;
				PxActor* pActor;
				BlastController& blastController = getBlastController();
				for (std::pair<int, std::set<int>> familychunksId : selection)
				{
					BlastFamily* pBlastFamily = blastController.getFamilyById(familychunksId.first);
					std::set<int>& chunkIds = familychunksId.second;
					for (int chunkId : chunkIds)
					{						
						pBlastFamily->getPxActorByChunkIndex(chunkId, &pActor);
						if (pActor != nullptr)
						{
							actors.emplace(pActor);
						}
					}
				}
				rangeSelect(actors, selectMode);
			}

			m_DrawSelectScreenPos.clear();
			m_ScreenRenderBuffer.clear();
			m_bRectSelecting = false;
			m_bDrawSelecting = false;
			m_bSelecting = false;
		}
		else if (uMsg == WM_RBUTTONUP)
		{
			/*
			if (m_actorsSelected.size() > 1)
			{
				return 1;
			}
			*/
			if (m_bDrawSelecting)
			{
				m_ScreenRenderBuffer.clear();

				DirectX::XMFLOAT4 LINE_COLOR(1.0f, 0.0f, 0.0f, 1.0f);

				int size = m_DrawSelectScreenPos.size() - 1;
				if (size == 0)
				{
					return 1;
				}

				PxVec3 from(0, 0, 0);
				PxVec3 to(0, 0, 0);
				for (int i = 0; i < size; i++)
				{
					from.x = m_DrawSelectScreenPos[i].x;
					from.y = m_DrawSelectScreenPos[i].y;
					to.x = m_DrawSelectScreenPos[i + 1].x;
					to.y = m_DrawSelectScreenPos[i + 1].y;
					m_ScreenRenderBuffer.m_lines.push_back(
						PxDebugLine(from, to, XMFLOAT4ToU32Color(LINE_COLOR)));
				}
				// connect tail and head to close
				from.x = m_DrawSelectScreenPos[0].x;
				from.y = m_DrawSelectScreenPos[0].y;
				m_ScreenRenderBuffer.m_lines.push_back(
					PxDebugLine(to, from, XMFLOAT4ToU32Color(LINE_COLOR)));
			}
		}
	}

	return 1;
}

void SelectionToolController::drawUI()
{
}

void SelectionToolController::pointSelect(PxActor* actor, SelectMode selectMode)
{
	std::set<PxActor*> actors;
	actors.emplace(actor);
	rangeSelect(actors, selectMode);
}

void SelectionToolController::rangeSelect(std::set<PxActor*>& actorsToSelect, SelectMode selectMode)
{
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