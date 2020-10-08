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
	int32_t*		getMaterialIds() override;

	/**
	Get loaded per triangle smoothing groups.  Currently not supported.
	*/
	int32_t*		getSmoothingGroups() override;

	/**
	Get material name.
	*/
	const char*		getMaterialName(int32_t id) override;


	int32_t			getMaterialCount() override;

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