/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "EditionToolController.h"
#include "BlastController.h"
#include "Renderer.h"
#include "PhysXController.h"
#include "SampleProfiler.h"
#include "PxRigidDynamic.h"
#include "PxScene.h"
#include "NvBlastExtPxManager.h"
#include "SceneController.h"
#include "NvBlastExtPxActor.h"
#include "GlobalSettings.h"
using namespace Nv::Blast;
using namespace physx;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Setup
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


EditionToolController::EditionToolController()
{
}

EditionToolController::~EditionToolController()
{
}

void EditionToolController::onSampleStart()
{
}

void EditionToolController::onInitialize()
{
}


void EditionToolController::onSampleStop() 
{
}

void EditionToolController::Animate(double dt)
{
	PROFILER_SCOPED_FUNCTION();
}


LRESULT EditionToolController::MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PROFILER_SCOPED_FUNCTION();

	if (uMsg == WM_LBUTTONDOWN || uMsg == WM_MOUSEMOVE || uMsg == WM_LBUTTONUP)
	{
		float mouseX = (short)LOWORD(lParam) / getRenderer().getScreenWidth();
		float mouseY = (short)HIWORD(lParam) / getRenderer().getScreenHeight();
		bool press = uMsg == WM_LBUTTONDOWN;

		if (uMsg == WM_LBUTTONUP)
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
			fracture(actor);
		}
	}

	return 1;
}

void EditionToolController::drawUI()
{
}

void EditionToolController::fracture(PxActor* actor)
{
	if (NULL == actor)
	{
		return;
	}

	BlastController& blastController = getBlastController();
	std::vector<BlastFamilyPtr>& families = blastController.getFamilies();
	if (families.size() == 0)
	{
		return;
	}

	ExtPxActor* extActor = NULL;
	PxRigidDynamic* rigidDynamic = actor->is<PxRigidDynamic>();
	if (NULL != rigidDynamic)
	{
		extActor = blastController.getExtPxManager().getActorFromPhysXActor(*rigidDynamic);
	}
	if (NULL == extActor)
	{
		return;
	}

	uint32_t chunkCount = extActor->getChunkCount();
	if (chunkCount <= 0)
	{
		return;
	}

	BlastFamilyPtr pBlastFamily = NULL;
	std::vector<BlastFamilyPtr>::iterator it = families.begin();
	for (; it != families.end(); it++)
	{
		BlastFamilyPtr f = *it;
		if (f->find(extActor))
		{
			pBlastFamily = f;
			break;
		}
	}
	if (NULL == pBlastFamily)
	{
		return;
	}

	const uint32_t* chunkIndices = extActor->getChunkIndices();

	const BlastAsset& blastAsset = pBlastFamily->getBlastAsset();
	const BlastAsset* pBlastAsset = &blastAsset;

	SceneController& sceneController = getManager()->getSceneController();
	AssetList::ModelAsset desc;
	sceneController.GetAssetDesc(pBlastAsset, desc);

	std::string assetname = desc.id;
	GlobalSettings& globalSettings = GlobalSettings::Inst();
	getManager()->fractureAsset(globalSettings.m_projectFileDir, assetname, pBlastAsset, chunkIndices[0]);
	getManager()->addModelAsset(globalSettings.m_projectFileDir, assetname, desc.isSkinned, desc.transform, false);
}