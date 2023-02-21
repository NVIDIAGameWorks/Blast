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
// Copyright (c) 2022-2023 NVIDIA Corporation. All rights reserved.


#ifndef NVBLASTEXTEXPORTERFBXWRITER_H
#define NVBLASTEXTEXPORTERFBXWRITER_H

#include "NvBlastExtExporter.h"
#include <memory>
#include <vector>
#include <map>

namespace fbxsdk
{
    class FbxScene;
    class FbxNode;
    class FbxMesh;
    class FbxSkin;
    class FbxManager;
    class FbxSurfaceMaterial;
    class FbxDisplayLayer;
}

struct NvBlastAsset;

namespace Nv
{
namespace Blast
{
class Mesh;
struct Triangle;
struct CollisionHull;

class FbxFileWriter : public IMeshFileWriter
{
public:

    /**
        Initialize FBX sdk and create scene.
    */
    FbxFileWriter();
    //~FbxFileWriter() = default;

    virtual void release() override;

    /**
        Get current scene;
    */
    fbxsdk::FbxScene* getScene();

    /**
        Append rendermesh to scene. Meshes constructed from arrays of triangles.
    */
    virtual bool appendMesh(const AuthoringResult& aResult, const char* assetName, bool nonSkinned) override;

    /**
        Append rendermesh to scene. Meshes constructed from arrays of vertex data (position, normal, uvs) and indices.
        Position, normal and uv has separate index arrays.
    */
    virtual bool appendMesh(const ExporterMeshData& meshData, const char* assetName, bool nonSkinned) override;

    /**
        Save scene to file.
    */
    virtual bool saveToFile(const char* assetName, const char* outputPath) override;

    /**
        Set interior material index.
    */
    virtual void setInteriorIndex(int32_t index) override;

    /**
        Set true if FBX should be saved in ASCII mode.
    */
    bool bOutputFBXAscii;

private:
    std::vector<fbxsdk::FbxSurfaceMaterial*> mMaterials;
    fbxsdk::FbxScene* mScene;
    fbxsdk::FbxDisplayLayer* mRenderLayer;

    //TODO we should track for every memory allocation and deallocate it not only for sdkManager
    std::shared_ptr<fbxsdk::FbxManager> sdkManager;
    std::map<uint32_t, fbxsdk::FbxNode*> chunkNodes;
    std::map<uint32_t, NvcVec3> worldChunkPivots;

    bool appendNonSkinnedMesh(const AuthoringResult& aResult, const char* assetName);
    bool appendNonSkinnedMesh(const ExporterMeshData& meshData, const char* assetName);
    void createMaterials(const ExporterMeshData& meshData);
    void createMaterials(const AuthoringResult& aResult);

    /**
    Append collision geometry to scene. Each node with collision geometry has "ParentalChunkIndex" property, which contain index of chunk
    which this collision geometry belongs to.
    */
    bool appendCollisionMesh(uint32_t meshCount, uint32_t* offsets, CollisionHull** hulls, const char* assetName);

    uint32_t addCollisionHulls(uint32_t chunkIndex, fbxsdk::FbxDisplayLayer* displayLayer, fbxsdk::FbxNode* parentNode, uint32_t hullsCount, CollisionHull** hulls);
    uint32_t createChunkRecursive(uint32_t currentCpIdx, uint32_t chunkIndex, fbxsdk::FbxNode *meshNode, fbxsdk::FbxNode* parentNode, fbxsdk::FbxSkin* skin, const AuthoringResult& aResult);
    uint32_t createChunkRecursive(uint32_t currentCpIdx, uint32_t chunkIndex, fbxsdk::FbxNode *meshNode, fbxsdk::FbxNode* parentNode, fbxsdk::FbxSkin* skin, const ExporterMeshData& meshData);

    void createChunkRecursiveNonSkinned(const std::string& meshName, uint32_t chunkIndex, fbxsdk::FbxNode* parentNode,
        const std::vector<fbxsdk::FbxSurfaceMaterial*>& materials, const AuthoringResult& aResult);

    void createChunkRecursiveNonSkinned(const std::string& meshName, uint32_t chunkIndex, fbxsdk::FbxNode* parentNode,
        const std::vector<fbxsdk::FbxSurfaceMaterial*>& materials, const ExporterMeshData& meshData);

    void addControlPoints(fbxsdk::FbxMesh* mesh, const ExporterMeshData& meshData);
    void addBindPose();

    void generateSmoothingGroups(fbxsdk::FbxMesh* mesh, FbxSkin* skin);
    void removeDuplicateControlPoints(fbxsdk::FbxMesh* mesh, FbxSkin* skin);

    int32_t mInteriorIndex;
};

}
}

#endif // NVBLASTEXTEXPORTERFBXWRITER_H