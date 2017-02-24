/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "SampleController.h"
#include "SceneController.h"
#include "CommonUIController.h"
#include "BlastController.h"
#include "PhysXController.h"

#include "imgui.h"

SampleController::SampleController()
{
}

SampleController::~SampleController()
{
}

void SampleController::onSampleStart()
{
	// start with GPU physics by default
	setUseGPUPhysics(true);
}


void SampleController::setUseGPUPhysics(bool useGPUPhysics)
{
	if (!getPhysXController().getGPUPhysicsAvailable())
	{
		useGPUPhysics = false;
	}

	if (getPhysXController().getUseGPUPhysics() == useGPUPhysics)
	{
		return;
	}

	int assetNum = getSceneController().releaseAll();

	getPhysXController().setUseGPUPhysics(useGPUPhysics);
	getBlastController().reinitialize();

	getSceneController().spawnAsset(assetNum);
}


void SampleController::drawPhysXGpuUI()
{
	// GPU Physics
	bool useGPU = getPhysXController().getUseGPUPhysics();
	if (ImGui::Checkbox("Use GPU Physics", &useGPU))
	{
		getCommonUIController().addDelayedCall([=]() { setUseGPUPhysics(useGPU); }, "Loading...");
	}
}