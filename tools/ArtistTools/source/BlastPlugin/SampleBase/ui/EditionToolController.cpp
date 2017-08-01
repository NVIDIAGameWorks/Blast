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
			getPhysXController().getEditPhysXScene().raycast(eyePos, pickDir, PX_MAX_F32, hit1, PxHitFlag::ePOSITION | PxHitFlag::eNORMAL);
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

	PxRigidDynamic* rigidDynamic = actor->is<PxRigidDynamic>();
	if (NULL == rigidDynamic)
	{
		return;
	}

	BlastFamilyPtr pBlastFamily = NULL;
	std::vector<BlastFamilyPtr>::iterator it = families.begin();
	for (; it != families.end(); it++)
	{
		BlastFamilyPtr f = *it;
		if (f->find(*actor))
		{
			pBlastFamily = f;
			break;
		}
	}
	if (NULL == pBlastFamily)
	{
		return;
	}

	const uint32_t chunkIndex = pBlastFamily->getChunkIndexByPxActor(*actor);

	const BlastAsset& blastAsset = pBlastFamily->getBlastAsset();
	BlastAsset* pBlastAsset = (BlastAsset*)&blastAsset;

	VoronoiFractureExecutor executor;
	executor.setSourceAsset(pBlastAsset);
	executor.setTargetChunk(chunkIndex);
	executor.execute();
}