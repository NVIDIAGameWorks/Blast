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


#ifndef NVBLASTEXTEXPORTEROBJWRITER_H
#define NVBLASTEXTEXPORTEROBJWRITER_H

#include "NvBlastExtExporter.h"
#include <memory>
#include <vector>
#include <foundation/PxVec2.h>
#include <foundation/PxVec3.h>
#include <string>
struct NvBlastAsset;

namespace Nv
{
namespace Blast
{

class ObjFileWriter : public IMeshFileWriter
{
public:

    ObjFileWriter(): mIntSurfaceMatIndex(-1), interiorNameStr("INTERIOR_MATERIAL") {  };
    ~ObjFileWriter() = default;

    virtual void release() override;

    virtual bool appendMesh(const AuthoringResult& aResult, const char* assetName, bool nonSkinned) override;

    /**
    Append rendermesh to scene. Meshes constructed from arrays of vertices and indices
    */
    virtual bool appendMesh(const ExporterMeshData& meshData, const char* assetName, bool nonSkinned) override;

    /**
    Save scene to file.
    */
    virtual bool saveToFile(const char* assetName, const char* outputPath) override;

    /**
        Set interior material index. Not supported in OBJ since AuthoringTool doesn't created OBJ with materials currently.
    */
    virtual void setInteriorIndex(int32_t index) override;

private:
    std::shared_ptr<ExporterMeshData> mMeshData;
    int32_t mIntSurfaceMatIndex;
    std::string interiorNameStr;
};

}
}

#endif // NVBLASTEXTEXPORTEROBJWRITER_H