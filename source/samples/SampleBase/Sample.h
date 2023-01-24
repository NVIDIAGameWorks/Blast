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


#ifndef SAMPLE_H
#define SAMPLE_H

#include "PxTransform.h"
#include <string>
#include <vector>


struct AssetList
{
    struct BoxAsset
    {
        BoxAsset() : staticHeight(-std::numeric_limits<float>().infinity()), 
            jointAllBonds(false), extents(20, 20, 20), bondFlags(7)
        {}

        struct Level
        {
            Level() :x(0), y(0), z(0), isSupport(0) {};

            int             x, y, z;
            bool            isSupport;
        };

        std::string         id;
        std::string         name;
        physx::PxVec3       extents;
        float               staticHeight;
        bool                jointAllBonds;
        std::vector<Level>  levels;
        uint32_t            bondFlags;
    };

    struct ModelAsset
    {
        ModelAsset() : isSkinned(false), transform(physx::PxIdentity) 
        {}

        std::string         id;
        std::string         file;
        std::string         name;
        physx::PxTransform  transform;
        bool                isSkinned;
    };

    struct CompositeAsset
    {
        CompositeAsset() : transform(physx::PxIdentity)
        {}

        struct AssetRef
        {
            std::string         id;
            physx::PxTransform  transform;
        };

        struct Joint
        {
            int32_t             assetIndices[2];
            uint32_t            chunkIndices[2];
            physx::PxVec3       attachPositions[2];
        };

        std::string             id;
        std::string             name;
        physx::PxTransform      transform;
        std::vector<AssetRef>   assetRefs;
        std::vector<Joint>      joints;
    };

    std::vector<ModelAsset>     models;
    std::vector<CompositeAsset> composites;
    std::vector<BoxAsset>       boxes;
};

struct SampleConfig
{
    std::wstring            sampleName;
    std::string             assetsFile;
    std::vector<std::string> additionalResourcesDir;
    AssetList               additionalAssetList;
};

int runSample(const SampleConfig& config);

#endif //SAMPLE_H