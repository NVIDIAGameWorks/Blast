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
// Copyright (c) 2016-2018 NVIDIA Corporation. All rights reserved.


#ifndef NVBLASTAUTHORINGTYPES_H
#define NVBLASTAUTHORINGTYPES_H

#include <PxVec3.h>
#include <PxVec2.h>
#include <PxBounds3.h>
#include "NvBlastTypes.h"

#define NOT_VALID_VERTEX INT32_MAX

namespace Nv
{
namespace Blast
{

/**
	Edge representation
*/
struct Edge
{
	uint32_t s, e;
	Edge() : s(NOT_VALID_VERTEX), e(NOT_VALID_VERTEX){}
	Edge(int s, int e) : s(s), e(e) {}
	bool operator<(const Edge& b) const
	{
		if (s == b.s)
			return e < b.e;
		else
			return s < b.s;
	}
};

/**
	Mesh vertex representation
*/
struct Vertex
{
	physx::PxVec3 p; // Position
	physx::PxVec3 n; // Normal
	physx::PxVec2 uv[1]; // UV-coordinates array, currently supported only one UV coordinate.
};


// Interior material ID
#define MATERIAL_INTERIOR 1000
#define SMOOTHING_GROUP_INTERIOR 1000



/**
	Mesh triangle representation
*/
struct Triangle
{
	Triangle() {};
	Triangle(Vertex a, Vertex b, Vertex c) : a(a), b(b), c(c) {};
	Vertex a, b, c;
	int32_t userData;
	int32_t materialId;
	int32_t smoothingGroup;
	physx::PxVec3 getNormal() const
	{
		return ((b.p - a.p).cross(c.p - a.p));
	}
	inline Vertex& getVertex(uint32_t index)
	{
		return (&a)[index];
	}
	inline const Vertex& getVertex(uint32_t index) const
	{
		return (&a)[index];
	}
};


/**
	Index based triangle
*/
struct TriangleIndexed
{
	TriangleIndexed() {};
	TriangleIndexed(uint32_t a, uint32_t b, uint32_t c) : ea(a), eb(b), ec(c) {};

	uint32_t getOpposite(uint32_t a, uint32_t b)
	{
		if (ea != a && ea != b)
			return ea;
		if (eb != a && eb != b)
			return eb;
		if (ec != a && ec != b)
			return ec;
		return NOT_VALID_VERTEX;
	}

	bool isContainEdge(uint32_t a, uint32_t b)
	{
		return (a == ea || a == eb || a == ec) && (b == ea || b == eb || b == ec);
	}

	Triangle convertToTriangle(Vertex* vertices)
	{
		Triangle tr;
		tr.a = vertices[ea];
		tr.b = vertices[eb];
		tr.c = vertices[ec];
		
		tr.userData = userData;
		tr.materialId = materialId;
		tr.smoothingGroup = smoothingGroup;
		return tr;
	}

	uint32_t ea, eb, ec;
	int32_t materialId;
	int32_t smoothingGroup;
	int32_t userData;
};




/**
	Mesh facet representation
*/
struct Facet
{
	int32_t		firstEdgeNumber;
	uint32_t	edgesCount;
	int64_t		userData;
	int32_t		materialId;
	int32_t		smoothingGroup;

	Facet(int32_t fEdge = 0, uint32_t eCount = 0, int32_t materialId = 0, int64_t userData = 0, int32_t smoothingGroup = 0) : firstEdgeNumber(fEdge), edgesCount(eCount), userData(userData), materialId(materialId), smoothingGroup(smoothingGroup) {}
};

/**
	Abstract base class for user-defined random value generator.
*/
class RandomGeneratorBase
{
public:
	// Generates uniformly distributed value in [0, 1] range. 
	virtual float	getRandomValue() = 0;
	// Seeds random value generator
	virtual void	seed(int32_t seed) = 0;
	virtual ~RandomGeneratorBase() {};
};

/**
	Collision hull geometry format.
*/
struct CollisionHull
{
	/**
	Collision hull polygon format.
	*/
	struct HullPolygon
	{
		// Polygon base plane
		float		mPlane[4];
		// Number vertices in polygon
		uint16_t	mNbVerts;
		// First index in CollisionHull.indices array for this polygon
		uint16_t	mIndexBase;
	};
	///**

	uint32_t		pointsCount;
	uint32_t		indicesCount;
	uint32_t		polygonDataCount;
	physx::PxVec3*	points;
	uint32_t*		indices;
	HullPolygon*	polygonData;

	virtual ~CollisionHull() {}

	virtual void release() = 0;
};

/**
	Authoring results. Which contains NvBlastAsset, render and collision meshes
*/
struct AuthoringResult
{
	uint32_t				chunkCount; //Number of chunks in Blast asset

	uint32_t				bondCount; //Number of bonds in Blast asset

	NvBlastAsset*			asset; //Blast asset

	/**
		assetToFractureChunkIdMap used for getting internal FractureChunkId with FractureTool::getChunkId.
		FractureChunkId = FractureTool.getChunkId(aResult.assetToFractureChunkIdMap(AssetChunkId);
	*/
	uint32_t*				assetToFractureChunkIdMap;

	/**
		Offsets for render mesh geometry. Contains chunkCount + 1 element.
		First triangle for i-th chunk: aResult.geometry[aResult.geometryOffset[i]]
		aResult.geometryOffset[chunkCount+1] is total number of triangles in geometry
	*/
	uint32_t*				geometryOffset;

	Triangle*				geometry; //Raw array of Triangle for all chunks

	NvBlastChunkDesc*		chunkDescs; //Array of chunk descriptors. Contains chunkCount elements

	NvBlastBondDesc*		bondDescs; //Array of bond descriptors. Contains bondCount elements

	/**
		Collision hull offsets. Contains chunkCount + 1 element.
		First collision hull for i-th chunk: aResult.collisionHull[aResult.collisionHullOffset[i]]
		aResult.collisionHullOffset[chunkCount+1] is total number of collision hulls in collisionHull
	*/
	uint32_t*				collisionHullOffset;

	CollisionHull**			collisionHull; //Raw array of pointers to collision hull for all chunks.

	/**
		Array of chunk physics parameters. Contains chunkCount elements
	*/
	struct ExtPxChunk*		physicsChunks;

	/**
		Array of phisics subchunks (convex mesh) descriptors. 
		Use collisionHullOffset for accessing elements.
	*/
	struct ExtPxSubchunk*	physicsSubchunks;

	/**
		Array of material names.
	*/
	const char** materialNames;
	/**
		Size of array of material names.
	*/

	uint32_t materialCount;

	//// Member functions ////
	virtual ~AuthoringResult() {}

	/**
		Free collision hulls data
	*/
	virtual void releaseCollisionHulls() = 0;

	/**
		Free all data and AuthoringResult
	*/
	virtual void release() = 0;
};


} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTAUTHORINGTYPES_H
