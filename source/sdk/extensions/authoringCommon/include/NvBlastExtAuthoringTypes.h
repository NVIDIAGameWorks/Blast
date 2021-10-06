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
// Copyright (c) 2016-2020 NVIDIA Corporation. All rights reserved.


#ifndef NVBLASTAUTHORINGTYPES_H
#define NVBLASTAUTHORINGTYPES_H

#include "NvBlastTypes.h"
#include "NvCTypes.h"

namespace Nv
{
namespace Blast
{
/**
Default material id assigned to interior faces (faces which created between 2 fractured chunks)
*/
const uint32_t kMaterialInteriorId       = 1000;

/**
Default smoothing group id assigned to interior faces
*/
const uint32_t kSmoothingGroupInteriorId = 1000;

/**
Vertex index which considired by NvBlast as not valid.
*/
const uint32_t kNotValidVertexIndex      = UINT32_MAX;

/**
Edge representation
*/
struct Edge
{
	Edge(uint32_t s = kNotValidVertexIndex, uint32_t e = kNotValidVertexIndex) : s(s), e(e) {}
	uint32_t s;
	uint32_t e;
};

/**
    Mesh vertex representation
*/
struct Vertex
{
	Vertex() {};
	Vertex(const NvcVec3& p, const NvcVec3& n, const NvcVec2& _uv) : p(p), n(n) { uv[0] = _uv; }
	NvcVec3 p;      // Position
	NvcVec3 n;      // Normal
	NvcVec2 uv[1];  // UV-coordinates array, currently supported only one UV coordinate.
};

/**
    Mesh triangle representation
*/
struct Triangle
{
	Triangle() {};
	Triangle(const Vertex& a, const Vertex& b, const Vertex& c, int32_t ud = 0, int32_t mid = 0, int32_t sid = 0) 
		: a(a), b(b), c(c), userData(ud), materialId(mid), smoothingGroup(sid) {}
	Vertex a, b, c;
	int32_t userData;
	int32_t materialId;
	int32_t smoothingGroup;
};

/**
    Index based triangle
*/
struct TriangleIndexed
{
	TriangleIndexed(uint32_t ea, uint32_t eb, uint32_t ec, int32_t mid = 0, int32_t sid = 0, int32_t ud = 0)
	: ea(ea), eb(eb), ec(ec), materialId(mid), smoothingGroup(sid), userData(ud) {}
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
	Facet(int32_t fen = 0, uint32_t ec = 0, int64_t ud = 0, int32_t mid = 0, int32_t sid = 0) 
		: firstEdgeNumber(fen), edgesCount(ec), userData(ud), materialId(mid), smoothingGroup(sid) {}
	int32_t firstEdgeNumber;
	uint32_t edgesCount;
	int64_t userData;
	int32_t materialId;
	int32_t smoothingGroup;
};

/**
    Collision hull geometry format.
*/
struct HullPolygon
{
	// Polygon base plane
	float plane[4];
	// Number vertices in polygon
	uint16_t vertexCount;
	// First index in CollisionHull.indices array for this polygon
	uint16_t indexBase;
};

/**
Collsion hull geometry.
*/
struct CollisionHull
{
	uint32_t pointsCount;
	uint32_t indicesCount;
	uint32_t polygonDataCount;
	NvcVec3* points;
	uint32_t* indices;
	HullPolygon* polygonData;
};

/**
    Authoring results. Which contains NvBlastAsset, render and collision meshes.
    If it was created by NvBlast it should be released with NvBlastExtAuthoringReleaseAuthoringResult
    For releasing just collsion geometry call NvBlastExtAuthoringReleaseAuthoringResultCollision
*/
struct AuthoringResult
{
	uint32_t chunkCount;  // Number of chunks in Blast asset

	uint32_t bondCount;  // Number of bonds in Blast asset

	NvBlastAsset* asset;  // Blast asset

	/**
	    assetToFractureChunkIdMap used for getting internal FractureChunkId with FractureTool::getChunkId.
	    FractureChunkId = FractureTool.getChunkId(aResult.assetToFractureChunkIdMap(AssetChunkId);
	*/
	uint32_t* assetToFractureChunkIdMap;

	/**
	    Offsets for render mesh geometry. Contains chunkCount + 1 element.
	    First triangle for i-th chunk: aResult.geometry[aResult.geometryOffset[i]]
	    aResult.geometryOffset[chunkCount+1] is total number of triangles in geometry
	*/
	uint32_t* geometryOffset;

	Triangle* geometry;  // Raw array of Triangle for all chunks

	NvBlastChunkDesc* chunkDescs;  // Array of chunk descriptors. Contains chunkCount elements

	NvBlastBondDesc* bondDescs;  // Array of bond descriptors. Contains bondCount elements

	/**
	    Collision hull offsets. Contains chunkCount + 1 element.
	    First collision hull for i-th chunk: aResult.collisionHull[aResult.collisionHullOffset[i]]
	    aResult.collisionHullOffset[chunkCount+1] is total number of collision hulls in collisionHull
	*/
	uint32_t* collisionHullOffset;

	CollisionHull** collisionHull;  // Raw array of pointers to collision hull for all chunks.

	/**
	    Array of material names.
	*/
	const char** materialNames;
	/**
	    Size of array of material names.
	*/

	uint32_t materialCount;
};

struct ConvexDecompositionParams
{
	uint32_t maximumNumberOfHulls = 8;  // Maximum number of convex hull generated for one chunk. If equal to 1 convex
	                                    // decomposition is disabled.
	uint32_t maximumNumberOfVerticesPerHull = 64;  // Controls the maximum number of triangles per convex-hull
	                                               // (default=64, range=4-1024)
	uint32_t voxelGridResolution = 1000000;        // Voxel grid resolution used for chunk convex decomposition
	                                               // (default=1,000,000, range=10,000-16,000,000).
	float concavity = 0.0025f;                     // Value between 0 and 1, controls how accurate hull generation is
};

}  // namespace Blast
}  // namespace Nv


#endif  // ifndef NVBLASTAUTHORINGTYPES_H
