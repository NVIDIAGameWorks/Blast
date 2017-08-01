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


#ifndef COMMON_UI_CONTROLLER_H
#define COMMON_UI_CONTROLLER_H

#include "SampleManager.h"
#include <DirectXMath.h>
#include <string>
#include <list>
#include <queue>
#include <functional>


class Renderer;
class PhysXController;
class BlastController;


class CommonUIController : public ISampleController
{
  public:
	CommonUIController();
	virtual ~CommonUIController() {};

	virtual HRESULT DeviceCreated(ID3D11Device* pDevice);
	virtual void DeviceDestroyed();
	virtual LRESULT MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void Animate(double fElapsedTimeSeconds);
	virtual void Render(ID3D11Device*, ID3D11DeviceContext*, ID3D11RenderTargetView*, ID3D11DepthStencilView*);

	void addDelayedCall(std::function<void()> func, const char* message)
	{
		addDelayedCall("PLEASE WAIT...", message, func);
	}

	void addPopupMessage(const char* title, const char* message, float duration = 2.f)
	{
		addDelayedCall(title, message, [] {}, duration);
	}

  private:
	void addDelayedCall(const char* title, const char* message, std::function<void()> func, float delay = 0.1f);

	void drawUI();
	void drawCodeProfiler(bool*);


	//////// used controllers ////////

	Renderer& getRenderer() const
	{
		return getManager()->getRenderer();
	}

	PhysXController& getPhysXController() const
	{
		return getManager()->getPhysXController();
	}

	BlastController&getBlastController() const
	{
		return getManager()->getBlastController();
	}

	DamageToolController& getDamageToolController() const
	{
		return getManager()->getDamageToolController();
	}

	SceneController& getSceneController() const
	{
		return getManager()->getSceneController();
	}

	SampleController& getSampleController() const
	{
		return getManager()->getSampleController();
	}


	//////// internal data ////////

	struct DelayedCall
	{
		std::function<void()>	func;
		const char*				title;
		const char*				message;
		float					delay;
		float					delayTotal;
	};

	std::queue<DelayedCall>		m_delayedCalls;

	float m_dt;

};

#endif