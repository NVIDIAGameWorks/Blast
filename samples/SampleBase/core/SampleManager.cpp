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
// Copyright (c) 2008-2020 NVIDIA Corporation. All rights reserved.


#include "SampleManager.h"

#include "Utils.h"

#include "Renderer.h"
#include "PhysXController.h"
#include "BlastController.h"
#include "CommonUIController.h"
#include "DamageToolController.h"
#include "SceneController.h"
#include "SampleController.h"


SampleManager::SampleManager(const SampleConfig& config) 
: m_config(config)
{
}

int SampleManager::run()
{
	Application app(getConfig().sampleName);

	m_renderer = new Renderer();
	m_physXController = new PhysXController(ExtImpactDamageManager::FilterShader);
	m_blastController = new BlastController();
	m_sceneController = new SceneController();
	m_damageToolController = new DamageToolController();
	m_sampleController = new SampleController();
	m_commonUIController = new CommonUIController();

	app.addControllerToFront(m_renderer);
	app.addControllerToFront(m_physXController);
	app.addControllerToFront(m_blastController);
	app.addControllerToFront(m_sceneController);
	app.addControllerToFront(m_damageToolController);
	app.addControllerToFront(m_sampleController);
	app.addControllerToFront(m_commonUIController);

	for (IApplicationController* c : app.getControllers())
	{
		(static_cast<ISampleController*>(c))->setManager(this);
	}

	int result = app.run();

	delete m_renderer;
	delete m_physXController;
	delete m_blastController;
	delete m_sceneController;
	delete m_damageToolController;
	delete m_sampleController;
	delete m_commonUIController;

	return result;
}


int runSample(const SampleConfig& config)
{
	SampleManager sampleManager(config);
	return sampleManager.run();
}