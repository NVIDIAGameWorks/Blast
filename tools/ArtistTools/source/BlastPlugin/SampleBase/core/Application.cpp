/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

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
