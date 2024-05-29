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

//! @file
//!
//! @brief Defines the API for the ExtPxStressSolver, which wraps the basic stress solver (NvBlastExtStress) for use with physx

#ifndef NVBLASTEXTPXSTRESSSOLVER_H
#define NVBLASTEXTPXSTRESSSOLVER_H

#include "NvBlastExtStressSolver.h"
#include "common/PxRenderBuffer.h"


namespace Nv
{
namespace Blast
{

// forward declarations
class ExtPxFamily;


/**
Px Stress Solver. Px wrapper over ExtStressSolver.

Uses ExtPxFamily and ExtStressSolver. see #ExtStressSolver for more details.
Works on both dynamic and static actor's within family.
For static actors it applies gravity.
For dynamic actors it applies centrifugal force.
*/
class NV_DLL_EXPORT ExtPxStressSolver
{
public:
    //////// creation ////////

    /**
    Create a new ExtStressSolver.

    \param[in]  family          The ExtPxFamily instance to calculate stress on.
    \param[in]  settings        The settings to be set on ExtStressSolver.

    \return the new ExtStressSolver if successful, NULL otherwise.
    */
    static ExtPxStressSolver*               create(ExtPxFamily& family, ExtStressSolverSettings settings = ExtStressSolverSettings());


    //////// interface ////////

    /**
    Release this stress solver.
    */
    virtual void                            release() = 0;

    /**
    Get actual ExtStressSolver used.

    \return the pointer to ExtStressSolver used internally.
    */
    virtual ExtStressSolver&                getSolver() const = 0;

    /**
    Update stress solver.

    Calculate stress and optionally apply damage.

    \param[in]  doDamage        If 'true' damage will be applied after stress solver.
    */
    virtual void                            update(bool doDamage = true) = 0;
};


} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTPXSTRESSSOLVER_H
