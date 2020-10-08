// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2020 NVIDIA Corporation. All rights reserved.


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