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
// Copyright (c) 2022-2024 NVIDIA Corporation. All rights reserved.


#ifndef NVBLASTEXTEXPORTERFBXREADER_H
#define NVBLASTEXTEXPORTERFBXREADER_H

#include <memory>
#include "fbxsdk.h"
#include <vector>
#include <map>
#include "NvBlastExtExporter.h"
#include "NvBlastExtAuthoringTypes.h"

namespace Nv
{
namespace Blast
{
class Mesh;

class FbxFileReader : public IFbxFileReader
{
    struct CollisionHullImpl : public Nv::Blast::CollisionHull
    {
        //copy from existing
        CollisionHullImpl(const CollisionHullImpl& other) : CollisionHullImpl()
        {
            copyFrom(other);
        }

        CollisionHullImpl()
        {
            pointsCount = 0;
            indicesCount = 0;
            polygonDataCount = 0;
            points = nullptr;
            indices = nullptr;
            polygonData = nullptr;
        }

        CollisionHullImpl(CollisionHullImpl&& other)
        {
            operator=(std::move(other));
        }

        CollisionHullImpl& operator=(const CollisionHullImpl& other)
        {
            if (&other != this)
            {
                delete[] points;
                delete[] indices;
                delete[] polygonData;
                copyFrom(other);
            }
            return *this;
        }

        CollisionHullImpl& operator=(CollisionHullImpl&& other)
        {
            if (&other != this)
            {
                pointsCount = other.pointsCount;
                indicesCount = other.indicesCount;
                polygonDataCount = other.polygonDataCount;
                points = other.points;
                indices = other.indices;
                polygonData = other.polygonData;

                other.pointsCount = 0;
                other.indicesCount = 0;
                other.polygonDataCount = 0;
                other.points = nullptr;
                other.indices = nullptr;
                other.polygonData = nullptr;
            }
            return *this;
        }

        virtual ~CollisionHullImpl()
        {
            delete[] points;
            delete[] indices;
            delete[] polygonData;
        }
    private:

        void copyFrom(const CollisionHullImpl& other)
        {
            pointsCount = other.pointsCount;
            indicesCount = other.indicesCount;
            polygonDataCount = other.polygonDataCount;
            points = new NvcVec3[pointsCount];
            indices = new uint32_t[indicesCount];
            polygonData = new Nv::Blast::HullPolygon[polygonDataCount];
            memcpy(points, other.points, sizeof(points[0]) * pointsCount);
            memcpy(indices, other.indices, sizeof(indices[0]) * indicesCount);
            memcpy(polygonData, other.polygonData, sizeof(polygonData[0]) * polygonDataCount);
        }
    };

public:
    FbxFileReader();
    ~FbxFileReader() = default;

    virtual void release() override;

    /*
    Load from the specified file path, returning a mesh or nullptr if failed
    */
    virtual void loadFromFile(const char* filename) override;

    virtual uint32_t getVerticesCount() const override
    {
        return mVertexPositions.size();
    }

    virtual uint32_t getIndicesCount() const override
    {
        return mIndices.size();
    }

    /**
    Check whether file contained an collision geometry
    */
    virtual bool isCollisionLoaded() override;

    /**
    Retrieve collision geometry if it exist
    */
    virtual uint32_t getCollision(uint32_t*& hullsOffset, Nv::Blast::CollisionHull**& hulls) override;

    virtual uint32_t getBoneInfluences(uint32_t*& out) override;

    virtual uint32_t getBoneCount() override;

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
    int32_t*        getMaterialIds() override;

    /**
    Get loaded per triangle smoothing groups.  Currently not supported.
    */
    int32_t*        getSmoothingGroups() override;

    /**
    Get material name.
    */
    const char*     getMaterialName(int32_t id) override;


    int32_t         getMaterialCount() override;

private:

    uint32_t mMeshCount;
    uint32_t mChunkCount;
    std::vector<uint32_t> mHullsOffset;
    std::vector<CollisionHullImpl> mHulls;
    std::vector<uint32_t> mVertexToContainingChunkMap;
    std::multimap<uint32_t, FbxNode*> mCollisionNodes;
    std::vector<NvcVec3> mVertexPositions;
    std::vector<NvcVec3> mVertexNormals;
    std::vector<NvcVec2> mVertexUv;
    std::vector<uint32_t> mIndices;
    std::vector<int32_t> mSmoothingGroups;
    std::vector<int32_t> mMaterialIds;
    std::vector<std::string> mMaterialNames;    
    
    FbxAMatrix getTransformForNode(FbxNode* node);
    void getFbxMeshes(FbxDisplayLayer* collisionDisplayLayer, FbxNode* node, std::vector<FbxNode*>& meshNodes);
    bool getCollisionInternal();
    bool getBoneInfluencesInternal(FbxMesh* meshNode);

};

}
}

#endif