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
// Copyright (c) 2022-2024 NVIDIA Corporation. All rights reserved.


#include "NvBlastExtSerialization.h"
#include "NvBlastExtTkSerializerRAW.h"
#include "NvBlastExtPxAsset.h"
#include "NvBlastTkAsset.h"
#include "NvBlastExtPxAssetImpl.h"
#include "NvBlastIndexFns.h"
#include "NvBlastAssert.h"
#include "NvBlastExtSerializationInternal.h"

#include "PxPhysics.h"
#include "PxIO.h"


namespace Nv
{
namespace Blast
{

// Legacy IDs
struct ExtPxSerializationLegacyID
{
    enum Enum
    {
        Asset = NVBLAST_FOURCC('B', 'P', 'X', 'A'), //!< ExtPxAsset identifier token, used in serialization
    };
};


// Legacy object format versions
struct ExtPxSerializationLegacyAssetVersion
{
    enum Enum
    {
        /** Initial version */
        Initial,

        //  New formats must come before Count.  They should be given descriptive names with more information in comments.

        /** The number of serialized formats. */
        Count,

        /** The current version.  This should always be Count-1 */
        Current = Count - 1
    };
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                          Helpers/Wrappers
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ExtIStreamToPxInputStream final : public PxInputStream
{
public:
    ExtIStreamToPxInputStream(ExtIStream& stream) : m_stream(stream) {}

    virtual uint32_t read(void* dest, uint32_t count) override
    {
        const size_t bytesToRead = m_stream.left() < count ? m_stream.left() : count;
        return m_stream.read(dest, bytesToRead) ? static_cast<uint32_t>(bytesToRead) : 0;
    }

private:
    ExtIStreamToPxInputStream& operator=(const ExtIStreamToPxInputStream&);

    ExtIStream& m_stream;
};


ExtPxAsset* deserializeExtPxAsset(ExtIStream& stream, TkFramework& framework, physx::PxPhysics& physics)
{
    // Read header
    struct LegacyAssetDataHeader
    {
        LegacyAssetDataHeader() : dataType(0), version(0) {}
        uint32_t dataType;
        uint32_t version;
    };
    LegacyAssetDataHeader header;
    stream >> header.dataType;
    stream >> header.version;
    NVBLAST_CHECK_ERROR(header.dataType == ExtPxSerializationLegacyID::Asset, "deserializeExtPxAsset: wrong data type in filebuf stream.", return nullptr);
    NVBLAST_CHECK_ERROR(header.version == ExtPxSerializationLegacyAssetVersion::Current, "deserializeExtPxAsset: wrong data version in filebuf stream.", return nullptr);

    // Read initial TkAsset
    TkAsset* tkAsset = deserializeTkAsset(stream, framework);
    NVBLAST_CHECK_ERROR(tkAsset != nullptr, "ExtPxAsset::deserialize: failed to deserialize TkAsset.", return nullptr);

    // Create ExtPxAsset
    ExtPxAssetImpl* asset = reinterpret_cast<ExtPxAssetImpl*>(ExtPxAsset::create(tkAsset));

    // Fill arrays
    auto& chunks = asset->getChunksArray();
    chunks.resize(tkAsset->getChunkCount());
    const uint32_t chunkCount = chunks.size();
    for (uint32_t i = 0; i < chunkCount; ++i)
    {
        ExtPxChunk& chunk = chunks[i];
        stream >> chunk.firstSubchunkIndex;
        stream >> chunk.subchunkCount;
        uint32_t val;
        stream >> val;
        chunk.isStatic = 0 != val;
    }

    auto& subchunks = asset->getSubchunksArray();
    uint32_t subchunkCount;
    stream >> subchunkCount;
    subchunks.resize(subchunkCount);
    for (uint32_t i = 0; i < subchunkCount; ++i)
    {
        ExtPxSubchunk& subchunk = subchunks[i];

        // Subchunk transform
        stream >> subchunk.transform.q.x >> subchunk.transform.q.y >> subchunk.transform.q.z >> subchunk.transform.q.w;
        stream >> subchunk.transform.p.x >> subchunk.transform.p.y >> subchunk.transform.p.z;

        // Subchunk scale
        stream >> subchunk.geometry.scale.scale.x >> subchunk.geometry.scale.scale.y >> subchunk.geometry.scale.scale.z;
        stream >> subchunk.geometry.scale.rotation.x >> subchunk.geometry.scale.rotation.y >> subchunk.geometry.scale.rotation.z >> subchunk.geometry.scale.rotation.w;

        uint32_t convexReuseIndex;
        stream >> convexReuseIndex;
        if (isInvalidIndex(convexReuseIndex))
        {
            ExtIStreamToPxInputStream inputStream(stream);
            subchunk.geometry.convexMesh = physics.createConvexMesh(inputStream);
        }
        else
        {
            NVBLAST_ASSERT_WITH_MESSAGE(convexReuseIndex < i, "ExtPxAsset::deserialize: wrong convexReuseIndex.");
            subchunk.geometry.convexMesh = subchunks[convexReuseIndex].geometry.convexMesh;
        }
        if (!subchunk.geometry.convexMesh)
        {
            NVBLAST_LOG_ERROR("ExtPxAsset::deserialize: failed to deserialize convex mesh.");
            return nullptr;
        }
    }

    // checking if it's the end, so it will be binary compatible with asset before m_defaultActorDesc was added
    if (!stream.eof())
    {
        auto& defaultActorDesc = asset->getDefaultActorDesc();

        stream >> defaultActorDesc.uniformInitialBondHealth;
        stream >> defaultActorDesc.uniformInitialLowerSupportChunkHealth;

        auto& bondHealths = asset->getBondHealthsArray();
        uint32_t bondHealthCount;
        stream >> bondHealthCount;
        bondHealths.resize(bondHealthCount);
        for (uint32_t i = 0; i < bondHealths.size(); ++i)
        {
            stream >> bondHealths[i];
        }
        defaultActorDesc.initialBondHealths = bondHealthCount ? bondHealths.begin() : nullptr;

        auto& supportChunkHealths = asset->getSupportChunkHealthsArray();
        uint32_t supportChunkHealthCount;
        stream >> supportChunkHealthCount;
        supportChunkHealths.resize(supportChunkHealthCount);
        for (uint32_t i = 0; i < supportChunkHealths.size(); ++i)
        {
            stream >> supportChunkHealths[i];
        }
        defaultActorDesc.initialSupportChunkHealths = supportChunkHealthCount ? supportChunkHealths.begin() : nullptr;
    }

    return asset;
}

}   // namespace Blast
}   // namespace Nv
