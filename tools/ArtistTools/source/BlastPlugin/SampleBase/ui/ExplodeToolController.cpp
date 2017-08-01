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


#include "ExplodeToolController.h"
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
#include "ViewerOutput.h"

using namespace Nv::Blast;
using namespace physx;

const float EXPLODE_MIN = 0.0f;
const float EXPLODE_MAX = 2.0f;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Setup
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


ExplodeToolController::ExplodeToolController()
	: m_previousExplodeValue(0.0f)
	, m_starDrag(false)
{
}

ExplodeToolController::~ExplodeToolController()
{
}

void ExplodeToolController::onSampleStart()
{
}

void ExplodeToolController::onInitialize()
{
	m_previousExplodeValue = 0.0f;
}


void ExplodeToolController::onSampleStop() 
{
}

void ExplodeToolController::Animate(double dt)
{
	PROFILER_SCOPED_FUNCTION();
}


LRESULT ExplodeToolController::MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PROFILER_SCOPED_FUNCTION();

	SampleManager* pSampleManager = SampleManager::ins();
	if (pSampleManager->IsSimulating())
	{
		return 1;
	}

	if (uMsg == WM_LBUTTONDOWN)
	{
		m_mouseStartPoint.x = (short)LOWORD(lParam);
		m_mouseStartPoint.y = (short)HIWORD(lParam);
		m_starDrag = true;
	}
	else if (uMsg == WM_MOUSEMOVE)
	{
		if (!m_starDrag)
			return 1;

		float theDragDistance = (short)LOWORD(lParam) - m_mouseStartPoint.x;
		// ignore shorter mouse move
		if (fabs(theDragDistance) <= 3.0)
			return 1;

		theDragDistance /= getRenderer().getScreenWidth();
		theDragDistance *= EXPLODE_MAX;
		float theNewExplodeValue = m_previousExplodeValue + (float)theDragDistance;

		if (theNewExplodeValue < 0)
			theNewExplodeValue = 0;
		else if (theNewExplodeValue > EXPLODE_MAX)
			theNewExplodeValue = EXPLODE_MAX;

		_explode(theNewExplodeValue);
	}
	else if (uMsg == WM_LBUTTONUP)
	{
		m_previousExplodeValue = 0.0f;
		m_starDrag = false;
	}
	return 1;
}

void ExplodeToolController::drawUI()
{
}

void ExplodeToolController::_explode(float value)
{
	SampleManager* pSampleManager = SampleManager::ins();

	BlastAsset* pBlastAsset = nullptr;
	int nFamilyIndex = -1;
	pSampleManager->getCurrentSelectedInstance(&pBlastAsset, nFamilyIndex);
	if (pBlastAsset == nullptr)
	{
		std::map<BlastAsset*, AssetList::ModelAsset>& AssetDescMap = pSampleManager->getAssetDescMap();
		if (AssetDescMap.size() == 1)
		{
			std::map<BlastAsset*, AssetList::ModelAsset>::iterator itAssetDescMap = AssetDescMap.begin();
			pSampleManager->setCurrentSelectedInstance(itAssetDescMap->first, -1);
			pBlastAsset = pSampleManager->getCurBlastAsset();
			viewer_msg("no asset selected, use the only one in current scene.");
		}
	}
	if (pBlastAsset == nullptr)
	{
		viewer_msg("please select one asset before explode.");
		return;
	}

	std::map<BlastAsset*, std::vector<BlastFamily*>>& AssetFamiliesMap = pSampleManager->getAssetFamiliesMap();
	std::map<BlastAsset*, std::vector<BlastFamily*>>::iterator itAFM = AssetFamiliesMap.find(pBlastAsset);
	if (itAFM == AssetFamiliesMap.end())
	{
		return;
	}

	std::vector<BlastFamily*> families = itAFM->second;
	int familySize = families.size();
	if (familySize == 0)
	{
		viewer_msg("no instance for current asset.");
		return;
	}

	if (nFamilyIndex == -1 || nFamilyIndex >= familySize)
	{
		nFamilyIndex = 0;
		viewer_msg("no instance selected, use the first one of current asset.");
	}

	BlastFamily* pFamily = families[nFamilyIndex];

	PxScene& scene = pSampleManager->getPhysXController().getEditPhysXScene();
	const PxU32 actorsCountTotal = scene.getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC);
	if (actorsCountTotal == 0)
	{
		return;
	}

	std::vector<PxActor*> actorsTotal(actorsCountTotal);
	PxU32 nbActors = scene.getActors(PxActorTypeFlag::eRIGID_DYNAMIC, &actorsTotal[0], actorsCountTotal, 0);
	PX_ASSERT(actorsCountTotal == nbActors);

	std::vector<PxActor*> actors;
	PxActor* pRootActor = nullptr;
	for (int act = 0; act < actorsCountTotal; act++)
	{
		if (pFamily->find(*actorsTotal[act]))
		{
			if (pRootActor == nullptr)
			{
				uint32_t chunkIndex = pFamily->getChunkIndexByPxActor(*actorsTotal[act]);
				std::vector<uint32_t> chunkIndexes;
				chunkIndexes.push_back(chunkIndex);
				std::vector<BlastChunkNode*> chunkNodes = BlastTreeData::ins().getChunkNodeByBlastChunk(pBlastAsset, chunkIndexes);
				if (chunkNodes.size() > 0 && BlastTreeData::isRoot(chunkNodes[0]))
				{
					pRootActor = actorsTotal[act];
				}
				else
				{
					actors.push_back(actorsTotal[act]);
				}
			}
			else
			{
				actors.push_back(actorsTotal[act]);
			}
		}
	}

	if (pRootActor == nullptr)
	{
		return;
	}

	BlastController& blastController = pSampleManager->getBlastController();

	PxVec3 origin = pRootActor->getWorldBounds().getCenter();

	int actorsCount = actors.size();
	for (int ac = 0; ac < actorsCount; ac++)
	{
		PxActor* actor = actors[ac];
		PxRigidDynamic* dynamic = actor->is<PxRigidDynamic>();
		PX_ASSERT(dynamic != nullptr);
		PxTransform transformOld = dynamic->getGlobalPose();
		PxTransform transformNew = transformOld;

		PxBounds3 bound = actor->getWorldBounds();
		PxVec3 target = bound.getCenter();

		//PxVec3 tChange = (target - origin) * value;
		//PxVec3 newTarget = target + tChange;
		//transformNew.p = transformOld.p + tChange;
		transformNew.p = (target - origin) * value;

		dynamic->setGlobalPose(transformNew);
		blastController.updateActorRenderableTransform(*actor, transformNew, false);
	}
}
