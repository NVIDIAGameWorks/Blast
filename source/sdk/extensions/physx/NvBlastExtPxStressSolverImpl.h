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
// Copyright (c) 2016-2022 NVIDIA Corporation. All rights reserved.


#ifndef NVBLASTEXTPXSTRESSSOLVERIMPL_H
#define NVBLASTEXTPXSTRESSSOLVERIMPL_H

#include "NvBlastExtPxStressSolver.h"
#include "NvBlastExtPxListener.h"
#include "NvBlastArray.h"
#include "NvBlastHashSet.h"

namespace Nv
{
namespace Blast
{


class ExtPxStressSolverImpl final : public ExtPxStressSolver, ExtPxListener
{
    NV_NOCOPY(ExtPxStressSolverImpl)

public:
    ExtPxStressSolverImpl(ExtPxFamily& family, ExtStressSolverSettings settings);


    //////// ExtPxStressSolver interface ////////

    virtual void                            release() override;

    virtual ExtStressSolver&                getSolver() const override
    {
        return *m_solver;
    }

    virtual void                            update(bool doDamage) override;


    //////// ExtPxListener interface ////////

    virtual void                            onActorCreated(ExtPxFamily& family, ExtPxActor& actor) final;

    virtual void                            onActorDestroyed(ExtPxFamily& family, ExtPxActor& actor) final;


private:
    ~ExtPxStressSolverImpl();


    //////// data ////////

    ExtPxFamily&                m_family;
    ExtStressSolver*            m_solver;
    HashSet<ExtPxActor*>::type  m_actors;
};


} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTPXSTRESSSOLVERIMPL_H
