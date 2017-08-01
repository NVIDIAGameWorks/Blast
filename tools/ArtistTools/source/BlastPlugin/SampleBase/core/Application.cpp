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


#include "Application.h"
#include <DirectXMath.h>
#include "XInput.h"
#include "DXUTMisc.h"


Application::Application(DeviceManager* pDeviceManager)
{
	m_deviceManager = pDeviceManager;
}

void Application::addControllerToFront(IApplicationController* controller)
{
	m_controllers.push_back(controller);
	m_deviceManager->AddControllerToFront(controller);
}

int Application::init()
{
	// FirstPersonCamera uses this timer, without it it will be FPS-dependent
	DXUTGetGlobalTimer()->Start();

	m_deviceManager->DeviceCreated();
	m_deviceManager->BackBufferResized();

	for (auto it = m_controllers.begin(); it != m_controllers.end(); it++)
		(*it)->onInitialize();

	for (auto it = m_controllers.begin(); it != m_controllers.end(); it++)
		(*it)->onSampleStart();

	m_deviceManager->SetVsyncEnabled(false);

	return 0;
}

int Application::run()
{
	m_deviceManager->MessageLoop();

	return 0;
}

int Application::free()
{
	for (auto it = m_controllers.rbegin(); it != m_controllers.rend(); it++)
		(*it)->onSampleStop();

	for (auto it = m_controllers.rbegin(); it != m_controllers.rend(); it++)
		(*it)->onTerminate();

	//m_deviceManager->Shutdown();   // destructor will call this function
	delete m_deviceManager;

	return 0;
}
