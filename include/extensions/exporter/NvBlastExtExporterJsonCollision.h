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
//! @brief Defines an API to serialize collision geometry to JSON format. 

#ifndef NVBLASTEXTEXPORTERJSONCOLLISION_H
#define NVBLASTEXTEXPORTERJSONCOLLISION_H

#include "NvBlastTypes.h"

namespace Nv
{
namespace Blast
{

struct CollisionHull;

/**
    Interface to object which serializes collision geometry to JSON format. 
*/
class IJsonCollisionExporter
{
public: 
    /**
        Delete this object
    */
    virtual void    release() = 0;

    /**
        Method creates file with given path and serializes given array of arrays of convex hulls to it in JSON format.
        \param[in] path         Output file path.
        \param[in] chunkCount   The number of chunks, may be less than the number of collision hulls.
        \param[in] hullOffsets  Collision hull offsets. Contains chunkCount + 1 element. First collision hull for i-th chunk: hull[hullOffsets[i]]. hullOffsets[chunkCount+1] is total number of hulls.
        \param[in] hulls        Array of pointers to convex hull descriptors, contiguously grouped for chunk[0], chunk[1], etc.
    */
    virtual bool    writeCollision(const char* path, uint32_t chunkCount, const uint32_t* hullOffsets, const CollisionHull* const * hulls) = 0;
};

} // namespace Blast
} // namespace Nv


/**
Creates an instance of IMeshFileWriter for writing obj file.
*/
NV_C_API Nv::Blast::IJsonCollisionExporter* NvBlastExtExporterCreateJsonCollisionExporter();


#endif //NVBLASTEXTEXPORTERJSONCOLLISION_H
