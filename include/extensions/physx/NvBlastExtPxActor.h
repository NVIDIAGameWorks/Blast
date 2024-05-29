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
//! @brief Defines the API for the NvBlastExtPxActor class

#ifndef NVBLASTEXTPXACTOR_H
#define NVBLASTEXTPXACTOR_H

#include "NvBlastTypes.h"


// Forward declarations
namespace physx
{
    class PxRigidDynamic;
}


namespace Nv
{
namespace Blast
{

// Forward declarations
class ExtPxFamily;
class TkActor;


/**
Actor.

Corresponds one to one to PxRigidDynamic and ExtActor.
*/
class ExtPxActor
{
public:
    /**
    Get the number of visible chunks for this actor.  May be used in conjunction with getChunkIndices().

    \return the number of visible chunk indices for the actor.
    */
    virtual uint32_t                getChunkCount() const = 0;

    /**
    Access actor's array of chunk indices. Use getChunkCount() to get a size of this array.

    \return a pointer to an array of chunk indices of an actor.
    */
    virtual const uint32_t*         getChunkIndices() const = 0;

    /**
    Every actor has corresponding PxActor.

    /return a pointer to PxRigidDynamic actor.
    */
    virtual physx::PxRigidDynamic&  getPhysXActor() const = 0;

    /**
    Every actor has corresponding TkActor.

    /return a pointer to TkActor actor.
    */
    virtual TkActor&                getTkActor() const = 0;

    /**
    Every actor has corresponding ExtPxFamily.

    /return a pointer to ExtPxFamily family.
    */
    virtual ExtPxFamily&            getFamily() const = 0;
};


} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTPXACTOR_H
