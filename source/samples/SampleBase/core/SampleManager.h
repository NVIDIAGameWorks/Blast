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
// Copyright (c) 2008-2022 NVIDIA Corporation. All rights reserved.


#ifndef SAMPLE_MANAGER_H
#define SAMPLE_MANAGER_H

#include "Application.h"
#include "Sample.h"


class SampleManager;

class ISampleController : public IApplicationController
{
public:

    void setManager(SampleManager* manager)
    {
        m_manager = manager;
    }
protected:

    SampleManager* getManager() const
    {
        return m_manager;
    }

private:
    SampleManager* m_manager;
};


class Renderer;
class PhysXController;
class BlastController;
class SceneController;
class DamageToolController;
class SampleController;
class CommonUIController;


/**
*/
class SampleManager
{
  public:
    SampleManager(const SampleConfig& config);
    int run();

    Renderer& getRenderer()
    {
        return *m_renderer;
    }

    PhysXController& getPhysXController() const
    {
        return *m_physXController;
    }

    BlastController& getBlastController() const
    {
        return *m_blastController;
    }

    SceneController& getSceneController() const
    {
        return *m_sceneController;
    }

    DamageToolController& getDamageToolController() const
    {
        return *m_damageToolController;
    }

    SampleController& getSampleController() const
    {
        return *m_sampleController;
    }

    CommonUIController& getCommonUIController() const
    {
        return *m_commonUIController;
    }

    const SampleConfig& getConfig() const
    {
        return m_config;
    }


  private:
      Renderer*             m_renderer;
      PhysXController*      m_physXController;
      BlastController*      m_blastController;
      SceneController*      m_sceneController;
      DamageToolController* m_damageToolController;
      SampleController*     m_sampleController;
      CommonUIController*   m_commonUIController;

      const SampleConfig&   m_config;
};


#endif