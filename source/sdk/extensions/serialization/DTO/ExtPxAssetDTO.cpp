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
// Copyright (c) 2022-2023 NVIDIA Corporation. All rights reserved.


#include "ExtPxAssetDTO.h"
#include "TkAssetDTO.h"
#include "ExtPxChunkDTO.h"
#include "ExtPxSubchunkDTO.h"
#include "physics/NvBlastExtPxAssetImpl.h"
#include "NvBlastAssert.h"
#include "NvBlast.h"


namespace Nv
{
namespace Blast
{

bool ExtPxAssetDTO::serialize(Nv::Blast::Serialization::ExtPxAsset::Builder builder, const Nv::Blast::ExtPxAsset * poco)
{
    TkAssetDTO::serialize(builder.getAsset(), &poco->getTkAsset());

    auto chunks = builder.initChunks(poco->getChunkCount());

    for (uint32_t i = 0; i <poco->getChunkCount(); i++)
    {
        ExtPxChunkDTO::serialize(chunks[i], &poco->getChunks()[i]);
    }

    auto subchunks = builder.initSubchunks(poco->getSubchunkCount());

    for (uint32_t i = 0; i < poco->getSubchunkCount(); i++)
    {
        ExtPxSubchunkDTO::serialize(subchunks[i], &poco->getSubchunks()[i]);
    }

    const NvBlastActorDesc& actorDesc = poco->getDefaultActorDesc();

    builder.setUniformInitialBondHealth(actorDesc.uniformInitialBondHealth);

    if (actorDesc.initialBondHealths != nullptr)
    {
        const uint32_t bondCount = poco->getTkAsset().getBondCount();
        kj::ArrayPtr<const float> bondHealthArray(actorDesc.initialBondHealths, bondCount);
        builder.initBondHealths(bondCount);
        builder.setBondHealths(bondHealthArray);
    }

    builder.setUniformInitialLowerSupportChunkHealth(actorDesc.uniformInitialLowerSupportChunkHealth);

    if (actorDesc.initialSupportChunkHealths != nullptr)
    {
        const uint32_t supportChunkCount = NvBlastAssetGetSupportChunkCount(poco->getTkAsset().getAssetLL(), logLL);
        kj::ArrayPtr<const float> supportChunkHealthArray(actorDesc.initialSupportChunkHealths, supportChunkCount);
        builder.initSupportChunkHealths(supportChunkCount);
        builder.setSupportChunkHealths(supportChunkHealthArray);
    }

    return true;
}


Nv::Blast::ExtPxAsset* ExtPxAssetDTO::deserialize(Nv::Blast::Serialization::ExtPxAsset::Reader reader)
{
    auto tkAsset = TkAssetDTO::deserialize(reader.getAsset());

    Nv::Blast::ExtPxAssetImpl* asset = reinterpret_cast<Nv::Blast::ExtPxAssetImpl*>(Nv::Blast::ExtPxAsset::create(tkAsset));

    NVBLAST_ASSERT(asset != nullptr);

    auto& chunks = asset->getChunksArray();
    const uint32_t chunkCount = reader.getChunks().size();
    chunks.resize(chunkCount);
    auto readerChunks = reader.getChunks();
    for (uint32_t i = 0; i < chunkCount; i++)
    {
        ExtPxChunkDTO::deserializeInto(readerChunks[i], &chunks[i]);
    }

    auto& subchunks = asset->getSubchunksArray();
    const uint32_t subChunkCount = reader.getSubchunks().size();
    subchunks.resize(subChunkCount);
    auto readerSubchunks = reader.getSubchunks();
    for (uint32_t i = 0; i < subChunkCount; i++)
    {
        ExtPxSubchunkDTO::deserializeInto(readerSubchunks[i], &subchunks[i]);
    }

    NvBlastActorDesc& actorDesc = asset->getDefaultActorDesc();

    actorDesc.uniformInitialBondHealth = reader.getUniformInitialBondHealth();

    actorDesc.initialBondHealths = nullptr;
    if (reader.hasBondHealths())
    {
        const uint32_t bondCount = asset->getTkAsset().getBondCount();
        Nv::Blast::Array<float>::type& bondHealths = asset->getBondHealthsArray();
        bondHealths.resize(bondCount);
        auto readerBondHealths = reader.getBondHealths();
        for (uint32_t i = 0; i < bondCount; ++i)
        {
            bondHealths[i] = readerBondHealths[i];
        }
    }

    actorDesc.uniformInitialLowerSupportChunkHealth = reader.getUniformInitialLowerSupportChunkHealth();

    actorDesc.initialSupportChunkHealths = nullptr;
    if (reader.hasSupportChunkHealths())
    {
        const uint32_t supportChunkCount = NvBlastAssetGetSupportChunkCount(asset->getTkAsset().getAssetLL(), logLL);
        Nv::Blast::Array<float>::type& supportChunkHealths = asset->getSupportChunkHealthsArray();
        supportChunkHealths.resize(supportChunkCount);
        auto readerSupportChunkHealths = reader.getSupportChunkHealths();
        for (uint32_t i = 0; i < supportChunkCount; ++i)
        {
            supportChunkHealths[i] = readerSupportChunkHealths[i];
        }
    }

    return asset;
}


bool ExtPxAssetDTO::deserializeInto(Nv::Blast::Serialization::ExtPxAsset::Reader reader, Nv::Blast::ExtPxAsset * poco)
{
    NV_UNUSED(reader);
    poco = nullptr;
    //NOTE: Because of the way this is structured, can't do this.
    return false;
}

}   // namespace Blast
}   // namespace Nv
