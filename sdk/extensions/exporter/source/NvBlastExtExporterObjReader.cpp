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
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.


#include "NvBlastExtExporterObjReader.h"

#pragma warning(push)
#pragma warning(disable:4706)
#pragma warning(disable:4702)
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#pragma warning(pop)


#include <iostream>
#include "PxVec3.h"
#include "PxVec2.h"
#include "NvBlastExtAuthoringMesh.h"

using physx::PxVec3;
using physx::PxVec2;
using namespace Nv::Blast;

ObjFileReader::ObjFileReader()
{
}

void ObjFileReader::release()
{
	delete this;
}

void ObjFileReader::loadFromFile(const char* filename)
{
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> mats;
	std::string err;
	std::string mtlPath;

	int32_t lastDelimeter = strlen(filename);
	
	while (lastDelimeter > 0 && filename[lastDelimeter] != '/' && filename[lastDelimeter] != '\\')
	{
		lastDelimeter--;
	}
	mtlPath = std::string(filename, filename + lastDelimeter);
	if (mtlPath == "")
	{
		mtlPath = '.';
	}
	mtlPath += '/';
	
	bool ret = tinyobj::LoadObj(shapes, mats, err, filename, mtlPath.c_str());
	
	// can't load?
	if (!ret)
	{
		return;
	}
	if (shapes.size() > 1)
	{
		std::cout << "Can load only one object per mesh" << std::endl;
	}

	if (!mats.empty())
	{
		if (mats.size() == 1 && mats[0].name == "")
		{
			mats[0].name = "Default";
		}
		for (uint32_t i = 0; i < mats.size(); ++i)
		{
				mMaterialNames.push_back(mats[i].name);
		}
	}

	mVertexPositions.clear();
	mVertexNormals.clear();
	mVertexUv.clear();
	mIndices.clear();

	auto& psVec = shapes[0].mesh.positions;
	for (uint32_t i = 0; i < psVec.size() / 3; ++i)
	{
		mVertexPositions.push_back(PxVec3(psVec[i * 3], psVec[i * 3 + 1], psVec[i * 3 + 2]));
	}
	auto& nmVec = shapes[0].mesh.normals;
	for (uint32_t i = 0; i < nmVec.size() / 3; ++i)
	{
		mVertexNormals.push_back(PxVec3(nmVec[i * 3], nmVec[i * 3 + 1], nmVec[i * 3 + 2]));
	}
	auto& txVec = shapes[0].mesh.texcoords;
	for (uint32_t i = 0; i < txVec.size() / 2; ++i)
	{
		mVertexUv.push_back(PxVec2(txVec[i * 2], txVec[i * 2 + 1]));
	}

	mIndices = shapes[0].mesh.indices;
	mPerFaceMatId = shapes[0].mesh.material_ids;
	for (uint32_t i = 0; i < mPerFaceMatId.size(); ++i)
	{
		if (mPerFaceMatId[i] == -1) // TinyOBJ loader sets ID to -1 when .mtl file not found. Set to default 0 material.
		{
			mPerFaceMatId[i] = 0;
		}
	}

}


bool ObjFileReader::isCollisionLoaded() 
{
	return false;
};


uint32_t ObjFileReader::getCollision(uint32_t*& hullsOffset, Nv::Blast::CollisionHull**& hulls)
{
	hullsOffset = nullptr;
	hulls = nullptr;
	return 0;
};

physx::PxVec3* ObjFileReader::getPositionArray()
{
	return mVertexPositions.data();
};

physx::PxVec3* ObjFileReader::getNormalsArray()
{
	return mVertexNormals.data();
};

physx::PxVec2* ObjFileReader::getUvArray()
{
	return mVertexUv.data();
};

uint32_t* ObjFileReader::getIndexArray()
{
	return mIndices.data();
};