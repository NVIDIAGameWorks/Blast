#pragma once
#include "IMeshFileWriter.h"
#include "fbxsdk.h"
#include <memory>

struct NvBlastAsset;

class FbxFileWriter : public IMeshFileWriter
{
public:

	FbxFileWriter();
	~FbxFileWriter() = default;

	virtual bool saveToFile(const NvBlastAsset* asset, std::vector<std::vector<Nv::Blast::Triangle>> chunksGeometry, std::string assetName, std::string outputPath) override;



	virtual bool saveToFile(const NvBlastAsset* asset, const std::string& name, const std::string& outputPath, const std::vector<physx::PxVec3>& pos, const std::vector<physx::PxVec3>& norm,
		const std::vector<physx::PxVec2>& uvs,
		const std::vector<std::vector<std::vector<int32_t> > >& posIndex,
		const std::vector<std::vector<std::vector<int32_t> > >& normIndex,
		const std::vector<std::vector<std::vector<int32_t> > >& texIndex,
		const std::vector<std::string>& texPathes,
		const uint32_t submeshCount) override;

	virtual bool saveToFile(const NvBlastAsset* asset, const std::string& name, const std::string& outputPath, const std::vector<physx::PxVec3>& pos, const std::vector<physx::PxVec3>& norm, const std::vector<physx::PxVec2>& uvs,
		const std::vector<std::vector<std::vector<int32_t> > >& indices) override;

	bool bOutputFBXAscii;

private:

	uint32_t currentDepth;

	std::shared_ptr<FbxManager> sdkManager;

	uint32_t createChunkRecursive(uint32_t currentCpIdx, uint32_t chunkIndex, FbxNode *meshNode, FbxNode* parentNode, FbxSkin* skin, const NvBlastAsset* asset, std::vector<std::vector<Nv::Blast::Triangle>> chunksGeometry);

	uint32_t createChunkRecursive(uint32_t currentCpIdx, uint32_t chunkIndex, FbxNode *meshNode, FbxNode* parentNode, FbxSkin* skin, const NvBlastAsset* asset,
		const std::vector<std::vector<std::vector<int32_t> > >& posIndex,
		const std::vector<std::vector<std::vector<int32_t> > >& normIndex,
		const std::vector<std::vector<std::vector<int32_t> > >& texIndex);

	void addControlPoints(FbxMesh* mesh, const std::vector<physx::PxVec3>& pos, std::vector<std::vector<std::vector<int32_t> > >& posIndex);
	
	bool finalizeFbxAndSave(FbxScene* scene, FbxSkin* skin, const std::string& outputPath, const std::string& name);
	
};
