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


#include "BlastFamilyBoxes.h"
#include "NvBlastExtPxAsset.h"
#include "NvBlastExtPxActor.h"
#include "BlastAssetBoxes.h"
#include "Renderer.h"
#include "PhysXController.h"
#include "RenderUtils.h"
#include "PxRigidDynamic.h"

using namespace physx;


BlastFamilyBoxes::BlastFamilyBoxes(PhysXController& physXController, ExtPxManager& pxManager, Renderer& renderer, const BlastAssetBoxes& blastAsset, const BlastAsset::ActorDesc& desc)
    : BlastFamily(physXController, pxManager, blastAsset), m_renderer(renderer)
{
    // prepare renderables
    IRenderMesh* boxRenderMesh = renderer.getPrimitiveRenderMesh(PrimitiveRenderMeshType::Box);
    RenderMaterial* primitiveRenderMaterial = physXController.getPrimitiveRenderMaterial();

    const ExtPxAsset* pxAsset = m_blastAsset.getPxAsset();
    const uint32_t chunkCount = pxAsset->getChunkCount();
    const ExtPxChunk* chunks = pxAsset->getChunks();
    const ExtPxSubchunk* subChunks = pxAsset->getSubchunks();
    m_chunkRenderables.resize(chunkCount);
    for (uint32_t i = 0; i < chunkCount; i++)
    {
        Renderable* renderable = renderer.createRenderable(*boxRenderMesh, *primitiveRenderMaterial);
        renderable->setHidden(true);
        renderable->setScale(subChunks[chunks[i].firstSubchunkIndex].geometry.scale.scale);
        m_chunkRenderables[i] = renderable;
    }

    // initialize in position
    initialize(desc);
}

BlastFamilyBoxes::~BlastFamilyBoxes()
{
    for (uint32_t i = 0; i < m_chunkRenderables.size(); i++)
    {
        m_renderer.removeRenderable(m_chunkRenderables[i]);
    }
}

void BlastFamilyBoxes::onActorCreated(const ExtPxActor& actor)
{
    DirectX::XMFLOAT4 color = getRandomPastelColor();

    const uint32_t* chunkIndices = actor.getChunkIndices();
    uint32_t chunkCount = actor.getChunkCount();
    for (uint32_t i = 0; i < chunkCount; i++)
    {
        const uint32_t chunkIndex = chunkIndices[i];
        m_chunkRenderables[chunkIndex]->setHidden(false);
        m_chunkRenderables[chunkIndex]->setColor(color);
    }
}

void BlastFamilyBoxes::onActorUpdate(const ExtPxActor& actor)
{
    const ExtPxChunk* chunks = m_blastAsset.getPxAsset()->getChunks();
    const ExtPxSubchunk* subChunks = m_blastAsset.getPxAsset()->getSubchunks();
    const uint32_t* chunkIndices = actor.getChunkIndices();
    uint32_t chunkCount = actor.getChunkCount();
    for (uint32_t i = 0; i < chunkCount; i++)
    {
        const uint32_t chunkIndex = chunkIndices[i];
        m_chunkRenderables[chunkIndex]->setTransform(actor.getPhysXActor().getGlobalPose() * subChunks[chunks[chunkIndex].firstSubchunkIndex].transform);
    }
}

void BlastFamilyBoxes::onActorDestroyed(const ExtPxActor& actor)
{
    const uint32_t* chunkIndices = actor.getChunkIndices();
    uint32_t chunkCount = actor.getChunkCount();
    for (uint32_t i = 0; i < chunkCount; i++)
    {
        m_chunkRenderables[chunkIndices[i]]->setHidden(true);
    }

}
