#pragma once
#include "NvBlastExtAuthoringTypes.h"
#include <string>
#include <memory>
#include <vector>

struct NvBlastAsset;

class IMeshFileWriter
{
public:

	/**
		Input mesh geometry as triangle array
	*/
	virtual bool saveToFile(const NvBlastAsset* asset, std::vector<std::vector<Nv::Blast::Triangle>> chunksGeometry, std::string assetName, std::string outputPath) = 0;


	/**
		Input mesh geometry as vertex buffers with separate indices for positions, normals and uvs. Is used for compressed output to .obj file.
	*/
	virtual bool saveToFile(const NvBlastAsset* asset, const std::string& name, const std::string& outputPath, const std::vector<physx::PxVec3>& pos, const std::vector<physx::PxVec3>& norm,
		const std::vector<physx::PxVec2>& uvs,
		const std::vector<std::vector<std::vector<int32_t> > >& posIndex,
		const std::vector<std::vector<std::vector<int32_t> > >& normIndex,
		const std::vector<std::vector<std::vector<int32_t> > >& texIndex,
		const std::vector<std::string>& texPathes,
		const uint32_t submeshCount) = 0;


	/**
		Input mesh geometry as vertex buffers and single index array.
	*/
	virtual bool saveToFile(const NvBlastAsset* asset, const std::string& name, const std::string& outputPath, const std::vector<physx::PxVec3>& pos, const std::vector<physx::PxVec3>& norm, const std::vector<physx::PxVec2>& uvs,
		const std::vector<std::vector<std::vector<int32_t> > >& indices) = 0;

	virtual bool getConvertToUE4() { return bConvertToUE4; }
	virtual void setConvertToUE4(bool bConvert) { bConvertToUE4 = bConvert; }
private:
	bool bConvertToUE4;
};
