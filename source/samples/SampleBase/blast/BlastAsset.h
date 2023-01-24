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


#ifndef BLAST_ASSET_H
#define BLAST_ASSET_H

#include <memory>
#include "PxTransform.h"
#include "NvBlastTypes.h"


using namespace physx;

class Renderer;
class BlastFamily;
class PhysXController;
class NvBlastExtDamageAccelerator;

namespace Nv
{
namespace Blast
{
class ExtPxFamily;
class ExtPxAsset;
class ExtPxManager;
class TkGroup;
}
}

using namespace Nv::Blast;

typedef std::shared_ptr<BlastFamily> BlastFamilyPtr;


class BlastAsset
{
public:
    //////// ctor ////////

    BlastAsset(Renderer& renderer);
    virtual ~BlastAsset();


    //////// desc ////////

    /**
    Descriptor with actor initial settings.
    */
    struct ActorDesc
    {
        NvBlastID           id;
        PxTransform         transform;
        TkGroup*            group;
    };


    //////// abstract ////////

    virtual BlastFamilyPtr createFamily(PhysXController& physXConroller, ExtPxManager& pxManager, const ActorDesc& desc) = 0;


    //////// data getters  ////////

    ExtPxAsset* getPxAsset() const
    { 
        return m_pxAsset;
    }

    size_t getBlastAssetSize() const;

    float getBondHealthMax() const
    {
        return m_bondHealthMax;
    }

    float getSupportChunkHealthMax() const
    {
        return m_supportChunkHealthMax;
    }

    NvBlastExtDamageAccelerator* getAccelerator() const
    {
        return m_damageAccelerator;
    }

protected:
    //////// internal operations ////////

    void initialize();


    //////// input data ////////

    Renderer&           m_renderer;


    //////// internal data ////////

    ExtPxAsset*                  m_pxAsset;
    float                        m_bondHealthMax;
    float                        m_supportChunkHealthMax;
    NvBlastExtDamageAccelerator* m_damageAccelerator;
};



#endif //BLAST_ASSET_H