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


#ifndef BLAST_ASSET_BOXES_H
#define BLAST_ASSET_BOXES_H

#include "BlastAsset.h"
#include "AssetGenerator.h"
#include "PxConvexMesh.h"


namespace physx
{
class PxPhysics;
class PxCooking;
}

namespace Nv
{
namespace Blast
{
class TkFramework;
}
}


class BlastAssetBoxes : public BlastAsset
{
public:
    struct Desc
    {
        CubeAssetGenerator::Settings generatorSettings;
        float staticHeight;
        bool jointAllBonds;
    };

    BlastAssetBoxes(TkFramework& framework, PxPhysics& physics, PxCooking& cooking, Renderer& renderer, const Desc& desc);
    virtual ~BlastAssetBoxes();

    BlastFamilyPtr createFamily(PhysXController& physXConroller, ExtPxManager& pxManager, const ActorDesc& desc);

private:
    PxConvexMesh*   m_boxMesh;
    GeneratorAsset  m_generatorAsset;
};



#endif //BLAST_ASSET_BOXES_H