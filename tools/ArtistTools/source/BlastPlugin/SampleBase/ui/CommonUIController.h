/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

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