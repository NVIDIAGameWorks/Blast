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


#ifndef NVBLASTEXTEXPORTEROBJREADER_H
#define NVBLASTEXTEXPORTEROBJREADER_H
#include <memory>
#include <string>
#include <vector>
#include "NvBlastExtExporter.h"

namespace Nv
{
namespace Blast
{
class Mesh;

class ObjFileReader : public IMeshFileReader
{
public:
    ObjFileReader();
    ~ObjFileReader() = default;

    virtual void release() override;

    /*
    Load from the specified file path, returning a mesh or nullptr if failed
    */
    virtual void loadFromFile(const char* filename) override;
    
    virtual uint32_t getVerticesCount() const override
    {
        return (uint32_t)mVertexPositions.size();
    }

    virtual uint32_t getIndicesCount() const override
    {
        return (uint32_t)mIndices.size();
    }

    /**
    Check whether file contained an collision geometry
    */
    virtual bool isCollisionLoaded() override;

    /**
    Retrieve collision geometry if it exist
    */
    virtual uint32_t getCollision(uint32_t*& hullsOffset, Nv::Blast::CollisionHull**& hulls) override;

    /**
        Get loaded vertex positions
    */
    virtual NvcVec3* getPositionArray() override;
    /**
        Get loaded vertex normals
    */
    virtual NvcVec3* getNormalsArray() override;
    /**
        Get loaded vertex uv-coordinates
    */
    virtual NvcVec2* getUvArray() override;
    /**
        Get loaded triangle indices
    */
    virtual uint32_t* getIndexArray() override;

    /**
        Get loaded per triangle material ids.
    */
    int32_t*        getMaterialIds() override { return mPerFaceMatId.data(); };

    /**
        Get loaded per triangle smoothing groups. Currently not supported by OBJ.
    */ 
    int32_t*        getSmoothingGroups() override { return nullptr; };

    /**
        Get material name.
    */
    const char*         getMaterialName(int32_t id) override { return mMaterialNames[id].c_str(); }

    /**
        Get material count.
    */
    int32_t     getMaterialCount() { return (uint32_t)mMaterialNames.size(); };

private:
    std::vector<NvcVec3>    mVertexPositions;
    std::vector<NvcVec3>    mVertexNormals;
    std::vector<NvcVec2>    mVertexUv;
    std::vector<uint32_t>       mIndices;

    std::vector<std::string>    mMaterialNames;
    std::vector<int32_t>        mPerFaceMatId;

};

}
}

#endif // NVBLASTEXTEXPORTEROBJREADER_H