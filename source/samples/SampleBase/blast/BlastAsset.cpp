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


#include "BlastAsset.h"
#include "NvBlastExtPxAsset.h"
#include "NvBlastTkAsset.h"
#include "NvBlastExtDamageShaders.h"
#include <algorithm>


BlastAsset::BlastAsset(Renderer& renderer)
    : m_renderer(renderer), m_bondHealthMax(1.0f), m_supportChunkHealthMax(1.0f), m_damageAccelerator(nullptr)
{
}

BlastAsset::~BlastAsset()
{
    if (m_damageAccelerator)
    {
        m_damageAccelerator->release();
    }
}

void BlastAsset::initialize()
{
    // calc max healths
    const auto& actorDesc = m_pxAsset->getDefaultActorDesc();
    if (actorDesc.initialBondHealths)
    {
        m_bondHealthMax = FLT_MIN;
        const uint32_t bondCount = m_pxAsset->getTkAsset().getBondCount();
        for (uint32_t i = 0; i < bondCount; ++i)
        {
            m_bondHealthMax = std::max<float>(m_bondHealthMax, actorDesc.initialBondHealths[i]);
        }
    }
    else
    {
        m_bondHealthMax = actorDesc.uniformInitialBondHealth;
    }

    if(actorDesc.initialSupportChunkHealths)
    {
        m_supportChunkHealthMax = FLT_MIN;
        const uint32_t nodeCount = m_pxAsset->getTkAsset().getGraph().nodeCount;
        for (uint32_t i = 0; i < nodeCount; ++i)
        {
            m_supportChunkHealthMax = std::max<float>(m_supportChunkHealthMax, actorDesc.initialSupportChunkHealths[i]);
        }
    }
    else
    {
        m_supportChunkHealthMax = actorDesc.uniformInitialLowerSupportChunkHealth;
    }

    m_damageAccelerator = NvBlastExtDamageAcceleratorCreate(m_pxAsset->getTkAsset().getAssetLL(), 3);
}

size_t BlastAsset::getBlastAssetSize() const
{
    return m_pxAsset->getTkAsset().getDataSize();
}
