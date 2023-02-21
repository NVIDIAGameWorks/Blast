// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2008-2023 NVIDIA Corporation. All rights reserved.


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
        std::function<void()>   func;
        const char*             title;
        const char*             message;
        float                   delay;
        float                   delayTotal;
    };

    std::queue<DelayedCall>     m_delayedCalls;

    float m_dt;

};

#endif