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
// Copyright (c) 2022 NVIDIA Corporation. All rights reserved.


#ifndef NVBLASTEXTEXPORTERFBXUTILS_H
#define NVBLASTEXTEXPORTERFBXUTILS_H

#include "fbxsdk.h"
#include <NvCTypes.h>
#include <string>

namespace Nv
{
    namespace Blast
    {
        struct Vertex;
    }
}

class FbxUtils
{
public:
    static void VertexToFbx(const Nv::Blast::Vertex& vert, FbxVector4& outVertex, FbxVector4& outNormal, FbxVector2& outUV);

    static void NvcVec3ToFbx(const NvcVec3& inVector, FbxVector4& outVector);
    static void NvcVec2ToFbx(const NvcVec2& inVector, FbxVector2& outVector);

    static FbxAxisSystem getBlastFBXAxisSystem();
    static FbxSystemUnit getBlastFBXUnit();

    static std::string FbxAxisSystemToString(const FbxAxisSystem& axisSystem);
    static std::string FbxSystemUnitToString(const FbxSystemUnit& systemUnit);

    //returns UINT32_MAX if not a chunk
    static uint32_t getChunkIndexForNode(FbxNode* node, uint32_t* outParentChunkIndex = nullptr);
    //Search using the old naming 
    static uint32_t getChunkIndexForNodeBackwardsCompatible(FbxNode* node, uint32_t* outParentChunkIndex = nullptr);
    static std::string getChunkNodeName(uint32_t chunkIndex);

    static std::string getCollisionGeometryLayerName();
    static std::string getRenderGeometryLayerName();
};

#endif //NVBLASTEXTEXPORTERFBXUTILS_H