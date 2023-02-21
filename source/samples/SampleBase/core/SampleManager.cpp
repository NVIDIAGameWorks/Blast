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


#include "SampleManager.h"

#include "Utils.h"

#include "Renderer.h"
#include "PhysXController.h"
#include "BlastController.h"
#include "CommonUIController.h"
#include "DamageToolController.h"
#include "SceneController.h"
#include "SampleController.h"


SampleManager::SampleManager(const SampleConfig& config) 
: m_config(config)
{
}

int SampleManager::run()
{
    Application app(getConfig().sampleName);

    m_renderer = new Renderer();
    m_physXController = new PhysXController(ExtImpactDamageManager::FilterShader);
    m_blastController = new BlastController();
    m_sceneController = new SceneController();
    m_damageToolController = new DamageToolController();
    m_sampleController = new SampleController();
    m_commonUIController = new CommonUIController();

    app.addControllerToFront(m_renderer);
    app.addControllerToFront(m_physXController);
    app.addControllerToFront(m_blastController);
    app.addControllerToFront(m_sceneController);
    app.addControllerToFront(m_damageToolController);
    app.addControllerToFront(m_sampleController);
    app.addControllerToFront(m_commonUIController);

    for (IApplicationController* c : app.getControllers())
    {
        (static_cast<ISampleController*>(c))->setManager(this);
    }

    int result = app.run();

    delete m_renderer;
    delete m_physXController;
    delete m_blastController;
    delete m_sceneController;
    delete m_damageToolController;
    delete m_sampleController;
    delete m_commonUIController;

    return result;
}


int runSample(const SampleConfig& config)
{
    SampleManager sampleManager(config);
    return sampleManager.run();
}