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


#include "BlastAssetModelSimple.h"
#include "BlastFamilyModelSimple.h"
#include "CustomRenderMesh.h"
#include "Renderer.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                              BlastAssetModelSimple
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BlastAssetModelSimple::BlastAssetModelSimple(TkFramework& framework, PxPhysics& physics, PxCooking& cooking, ExtSerialization& serialization, Renderer& renderer, const char* modelName)
    : BlastAssetModel(framework, physics, cooking, serialization, renderer, modelName)
{
    // prepare materials
    for (const BlastModel::Material& material : getModel().materials)
    {
        if (material.diffuseTexture.empty())
            m_renderMaterials.push_back(new RenderMaterial(renderer.getResourceManager(), "model_simple"));
        else
            m_renderMaterials.push_back(new RenderMaterial(renderer.getResourceManager(), "model_simple_textured", material.diffuseTexture.c_str()));
    }

    initialize();
}


BlastAssetModelSimple::~BlastAssetModelSimple()
{
    // release materials
    for (RenderMaterial* r : m_renderMaterials)
    {
        SAFE_DELETE(r);
    }
}


BlastFamilyPtr BlastAssetModelSimple::createFamily(PhysXController& physXConroller, ExtPxManager& pxManager, const ActorDesc& desc)
{
    return BlastFamilyPtr(new BlastFamilyModelSimple(physXConroller, pxManager, m_renderer, *this, desc));
}
