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
// Copyright (c) 2008-2024 NVIDIA Corporation. All rights reserved.


#ifndef SCENE_CONTROLLER_H
#define SCENE_CONTROLLER_H

#include "SampleManager.h"
#include <map>


class CFirstPersonCamera;
class BlastAssetBoxes;
class SceneActor;
class BlastAsset;
class SingleSceneAsset;
class Scene;

class SceneController : public ISampleController
{
public:

    SceneController();
    virtual ~SceneController();

    virtual LRESULT MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual void Animate(double dt);
    void drawUI();
    void drawStatsUI();

    virtual void onInitialize();
    virtual void onSampleStart();
    virtual void onSampleStop();
    virtual void onTerminate();

    // commands
    int releaseAll();
    void spawnAsset(int32_t);


private:
    void addAssets(const AssetList& assetList, bool loadModels = true);
    void throwCube();
    float getCubeSpeed();

    SceneController& operator= (SceneController&);

    //////// used controllers ////////

    Renderer& getRenderer() const
    {
        return getManager()->getRenderer();
    }

    PhysXController& getPhysXController() const
    {
        return getManager()->getPhysXController();
    }

    BlastController& getBlastController() const
    {
        return getManager()->getBlastController();
    }

    CommonUIController& getCommonUIController() const
    {
        return getManager()->getCommonUIController();
    }


    //////// internal data ////////

    Scene* m_scene;

    float m_cubeScale;
    float m_cubeThrowDownTime;
};

#endif