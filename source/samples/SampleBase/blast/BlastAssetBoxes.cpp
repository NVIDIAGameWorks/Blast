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


#include "BlastAssetBoxes.h"
#include "BlastFamilyBoxes.h"
#include "NvBlastExtPxAsset.h"
#include "PxPhysics.h"
#include "cooking/PxCooking.h"


BlastAssetBoxes::BlastAssetBoxes(TkFramework& framework, PxPhysics& physics, PxCooking& cooking, Renderer& renderer, const Desc& desc)
    : BlastAsset(renderer)
{
    // generate boxes slices procedurally
    CubeAssetGenerator::generate(m_generatorAsset, desc.generatorSettings);

    // asset desc / tk asset
    ExtPxAssetDesc assetDesc;
    assetDesc.chunkDescs = m_generatorAsset.solverChunks.data();
    assetDesc.chunkCount = (uint32_t)m_generatorAsset.solverChunks.size();
    assetDesc.bondDescs = m_generatorAsset.solverBonds.data();
    assetDesc.bondCount = (uint32_t)m_generatorAsset.solverBonds.size();
    std::vector<uint8_t> bondFlags(assetDesc.bondCount);
    std::fill(bondFlags.begin(), bondFlags.end(), desc.jointAllBonds ? 1 : 0);
    assetDesc.bondFlags = bondFlags.data();

    // box convex
    PxVec3 vertices[8] = { { -1, -1, -1 }, { -1, -1, 1 }, { -1, 1, -1 }, { -1, 1, 1 }, { 1, -1, -1 }, { 1, -1, 1 }, { 1, 1, -1 }, { 1, 1, 1 } };
    PxConvexMeshDesc convexMeshDesc;
    convexMeshDesc.points.count = 8;
    convexMeshDesc.points.data = vertices;
    convexMeshDesc.points.stride = sizeof(PxVec3);
    convexMeshDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;
    m_boxMesh = cooking.createConvexMesh(convexMeshDesc, physics.getPhysicsInsertionCallback());

    // prepare chunks
    const uint32_t chunkCount = (uint32_t)m_generatorAsset.solverChunks.size();
    std::vector<ExtPxAssetDesc::ChunkDesc> pxChunks(chunkCount);
    std::vector<ExtPxAssetDesc::SubchunkDesc> pxSubchunks;
    pxSubchunks.reserve(chunkCount);
    for (uint32_t i = 0; i < m_generatorAsset.solverChunks.size(); i++)
    {
        uint32_t chunkID = m_generatorAsset.solverChunks[i].userData;
        GeneratorAsset::BlastChunkCube& cube = m_generatorAsset.chunks[chunkID];
        PxVec3 position = *reinterpret_cast<PxVec3*>(&cube.position);
        PxVec3 extents = *reinterpret_cast<PxVec3*>(&cube.extents);
        ExtPxAssetDesc::ChunkDesc& chunk = pxChunks[chunkID];
        ExtPxAssetDesc::SubchunkDesc subchunk =
        {
            PxTransform(position),
            PxConvexMeshGeometry(m_boxMesh, PxMeshScale(extents / 2))
        };
        pxSubchunks.push_back(subchunk);
        chunk.subchunks = &pxSubchunks.back();
        chunk.subchunkCount = 1;
        chunk.isStatic = (position.y - (extents.y - desc.generatorSettings.extents.y) / 2) <= desc.staticHeight;
    }

    // create asset
    assetDesc.pxChunks = pxChunks.data();
    m_pxAsset = ExtPxAsset::create(assetDesc, framework);

    initialize();
}


BlastAssetBoxes::~BlastAssetBoxes()
{
    m_boxMesh->release();
    m_pxAsset->release();
}


BlastFamilyPtr BlastAssetBoxes::createFamily(PhysXController& physXConroller, ExtPxManager& pxManager, const ActorDesc& desc)
{
    return BlastFamilyPtr(new BlastFamilyBoxes(physXConroller, pxManager, m_renderer, *this, desc));
}
