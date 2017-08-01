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