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
	std::string	interiorNameStr;
};

}
}

#endif // NVBLASTEXTEXPORTEROBJWRITER_H