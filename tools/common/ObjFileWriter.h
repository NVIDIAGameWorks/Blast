#pragma once
#include "IMeshFileWriter.h"
#include <memory>

struct NvBlastAsset;

class ObjFileWriter : public IMeshFileWriter
{
public:

	ObjFileWriter() {};
	~ObjFileWriter() = default;

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
};
