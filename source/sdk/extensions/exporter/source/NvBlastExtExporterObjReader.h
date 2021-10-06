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
	int32_t*		getMaterialIds() override { return mPerFaceMatId.data(); };

	/**
		Get loaded per triangle smoothing groups. Currently not supported by OBJ.
	*/ 
	int32_t*		getSmoothingGroups() override { return nullptr; };

	/**
		Get material name.
	*/
	const char*			getMaterialName(int32_t id) override { return mMaterialNames[id].c_str(); }

	/**
		Get material count.
	*/
	int32_t		getMaterialCount() { return mMaterialNames.size(); };

private:
	std::vector<NvcVec3>	mVertexPositions;
	std::vector<NvcVec3>	mVertexNormals;
	std::vector<NvcVec2>	mVertexUv;
	std::vector<uint32_t>		mIndices;

	std::vector<std::string>	mMaterialNames;
	std::vector<int32_t>		mPerFaceMatId;

};

}
}

#endif // NVBLASTEXTEXPORTEROBJREADER_H