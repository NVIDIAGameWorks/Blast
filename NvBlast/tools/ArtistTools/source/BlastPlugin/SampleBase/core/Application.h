/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef APPLICATION_H
#define APPLICATION_H

#include <DeviceManager.h>
#include <vector>
#include <string>

/**
ISampleController adds more onstart and onstop callbacks to IVisualController
*/
class IApplicationController : public IVisualController
{
  public:
	virtual void onInitialize() {}
	virtual void onSampleStart() {}
	virtual void onSampleStop() {}
	virtual void onTerminate() {}
};


/**
Main manager which runs sample.
You have to add controllers to it which will receive all the start, animate, render etc. callbacks.
*/
class Application
{
public:
	Application(DeviceManager* pDeviceManager);
	void addControllerToFront(IApplicationController* controller);

	const std::vector<IApplicationController*>& getControllers() const 
	{ 
		return m_controllers; 
	}

	int init();
	int run();
	int free();

private:
	DeviceManager* m_deviceManager;
	std::vector<IApplicationController*> m_controllers;
//	std::wstring m_sampleName;
};


#endif //APPLICATION_H