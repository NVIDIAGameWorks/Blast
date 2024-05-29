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
// Copyright (c) 2016-2024 NVIDIA Corporation. All rights reserved.


#ifndef NVBLASTEXTPXACTORIMPL_H
#define NVBLASTEXTPXACTORIMPL_H

#include "NvBlastExtPxActor.h"
#include "NvBlastArray.h"
#include "foundation/PxTransform.h"


using namespace physx;

namespace Nv
{
namespace Blast
{


// Forward declarations
class ExtPxFamilyImpl;

struct PxActorCreateInfo
{
    PxTransform m_transform;
    PxVec3      m_scale;
    PxVec3      m_parentLinearVelocity;
    PxVec3      m_parentAngularVelocity;
    PxVec3      m_parentCOM;
};


class ExtPxActorImpl final : public ExtPxActor
{
public:
    //////// ctor ////////

    ExtPxActorImpl(ExtPxFamilyImpl* family, TkActor* tkActor, const PxActorCreateInfo& pxActorInfo);

    ~ExtPxActorImpl()
    {
        release();
    }

    void release();


    //////// interface ////////

    virtual uint32_t                    getChunkCount() const override
    {
        return static_cast<uint32_t>(m_chunkIndices.size());
    }

    virtual const uint32_t*             getChunkIndices() const override
    {
        return m_chunkIndices.begin();
    }

    virtual PxRigidDynamic&             getPhysXActor() const override
    {
        return *m_rigidDynamic;
    }

    virtual TkActor&                    getTkActor() const override
    {
        return *m_tkActor;
    }

    virtual ExtPxFamily&                getFamily() const override;


private:
    //////// data ////////

    ExtPxFamilyImpl*                    m_family;
    TkActor*                            m_tkActor;
    PxRigidDynamic*                     m_rigidDynamic;
    InlineArray<uint32_t, 4>::type      m_chunkIndices;
};



} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTPXACTORIMPL_H
