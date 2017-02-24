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


Application::Application(std::wstring sampleName)
: m_sampleName(sampleName)
{
	m_deviceManager = new DeviceManager();
}

void Application::addControllerToFront(IApplicationController* controller)
{
	m_controllers.push_back(controller);
}

int Application::run()
{
	// FirstPersonCamera uses this timer, without it it will be FPS-dependent
	DXUTGetGlobalTimer()->Start();

	for (auto it = m_controllers.begin(); it != m_controllers.end(); it++)
		m_deviceManager->AddControllerToFront(*it);

	DeviceCreationParameters deviceParams;
	deviceParams.swapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	deviceParams.swapChainSampleCount = 4;
	deviceParams.startFullscreen = false;
	deviceParams.backBufferWidth = 1600;
	deviceParams.backBufferHeight = 900;
#if defined(DEBUG) | defined(_DEBUG)
	deviceParams.createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;
#endif
	deviceParams.featureLevel = D3D_FEATURE_LEVEL_11_0;

	if (FAILED(m_deviceManager->CreateWindowDeviceAndSwapChain(deviceParams, m_sampleName.c_str())))
	{
		MessageBoxA(nullptr, "Cannot initialize the D3D11 device with the requested parameters", "Error",
			MB_OK | MB_ICONERROR);
		return 1;
	}

	for (auto it = m_controllers.begin(); it != m_controllers.end(); it++)
		(*it)->onInitialize();

	for (auto it = m_controllers.begin(); it != m_controllers.end(); it++)
		(*it)->onSampleStart();

	m_deviceManager->SetVsyncEnabled(false);
	m_deviceManager->MessageLoop();

	for (auto it = m_controllers.rbegin(); it != m_controllers.rend(); it++)
		(*it)->onSampleStop();

	for (auto it = m_controllers.rbegin(); it != m_controllers.rend(); it++)
		(*it)->onTerminate();

	m_deviceManager->Shutdown();
	delete m_deviceManager;

	return 0;
}
