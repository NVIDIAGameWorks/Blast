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
// Copyright (c) 2023 NVIDIA Corporation. All rights reserved.

//! @file
//!
//! @brief Defines blast physx (Px) serialization support for the NvBlastExtSerialization blast extension

#pragma once

#include "NvBlastGlobals.h"


/**
Blast serialization support for the ExtPhysX extension.  Contains serializers which can be used by the ExtSerialization manager.
*/


namespace Nv
{
namespace Blast
{

// Forward declarations
class TkFramework;
class ExtSerialization;
class ExtPxAsset;


/** Standard Object Type IDs */
struct ExtPxObjectTypeID
{
    enum Enum
    {
        Asset = NVBLAST_FOURCC('P', 'X', 'A', 'S'),
    };
};

}   // namespace Blast
}   // namespace Nv


namespace physx
{

// Forward declarations
class PxPhysics;
class PxCooking;

}   // namespace physx


/**
Load all ExtPhysX extension serializers into the ExtSerialization manager.

It does no harm to call this function more than once; serializers already loaded will not be loaded again.

\param[in]  serialization   Serialization manager into which to load serializers.

\return the number of serializers loaded.
*/
NV_C_API size_t      NvBlastExtPxSerializerLoadSet(Nv::Blast::TkFramework& framework, physx::PxPhysics& physics, physx::PxCooking& cooking, Nv::Blast::ExtSerialization& serialization);


/**
Utility wrapper function to serialize an ExtPxAsset.  Allocates the buffer internally using the
callack set in ExtSerialization::setBufferProvider.

Equivalent to:

    serialization.serializeIntoBuffer(buffer, asset, Nv::Blast::ExtPxObjectTypeID::Asset);

\param[out] buffer          Pointer to the buffer created.
\param[in]  serialization   Serialization manager.
\param[in]  asset           Pointer to the ExtPxAsset to serialize.

\return the number of bytes serialized into the buffer (zero if unsuccessful).
*/
NV_C_API uint64_t    NvBlastExtSerializationSerializeExtPxAssetIntoBuffer(void*& buffer, Nv::Blast::ExtSerialization& serialization, const Nv::Blast::ExtPxAsset* asset);
