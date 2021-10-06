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


#ifndef NVBLASTEXTAUTHORINGTRIANGULATOR_H
#define NVBLASTEXTAUTHORINGTRIANGULATOR_H


#include <vector>
#include <map>
#include "NvBlastExtAuthoringTypes.h"
#include "NvBlastExtAuthoringMesh.h"
#include "NvBlastExtAuthoringInternalCommon.h"

namespace Nv
{
namespace Blast
{


/**
	Tool for doing all post processing steps of authoring.
*/
class Triangulator
{
public:
	/**
		Triangulates provided mesh and saves result internally. Uses Ear-clipping algorithm.
		\param[in] mesh Mesh for triangulation
	*/
	void							triangulate(const Mesh* mesh);

	/**
		\return Return array of triangles of base mesh.
	*/
	std::vector<Triangle>&			getBaseMesh()
	{
		return mBaseMeshUVFittedTriangles;
	}

	std::vector<Triangle>&			getBaseMeshNotFitted()
	{
		return mBaseMeshResultTriangles;
	}


	/**
		\return Return array of TriangleIndexed of base mesh. Each TriangleIndexed contains index of corresponding vertex in internal vertex buffer.
	*/
	std::vector<TriangleIndexed>&	getBaseMeshIndexed()
	{
		return mBaseMeshTriangles;
	}
	/**
		\return Return mapping from vertices of input Mesh to internal vertices buffer. Used for island detection.
	*/
	std::vector<uint32_t>&			getBaseMapping()
	{
		return mBaseMapping;
	};
	/**
		\return Return mapping from vertices of input Mesh to internal vertices buffer, only positions are accounted. Used for island detection. 
	*/
	std::vector<int32_t>&			getPositionedMapping()
	{
		return mPositionMappedVrt;
	};
	/**
		\return Return internal vertex buffer size. Vertices internally are welded with some threshold.
	*/
	uint32_t						getWeldedVerticesCount()
	{
		return static_cast<uint32_t>(mVertices.size());
	}	

	/**
		Removes all information about mesh triangulation.
	*/
	void							reset();

	int32_t&						getParentChunkId() { return parentChunkId; };

private:

	int32_t							parentChunkId;

	int32_t							addVerticeIfNotExist(const Vertex& p);
	void							addEdgeIfValid(EdgeWithParent& ed);
	
	/* Data used before triangulation to build polygon loops*/

	std::vector<Vertex>									mVertices;
	std::vector<EdgeWithParent>							mBaseMeshEdges;
	std::map<Vertex, int32_t, VrtComp>					mVertMap;
	std::map<EdgeWithParent, int32_t, EdgeComparator>	mEdgeMap;
	std::vector<uint32_t>								mBaseMapping;
	std::vector<int32_t>								mPositionMappedVrt;
	/* ------------------------------------------------------------ */


	/**
		Unite all almost similar vertices, update edges according to this changes
	*/
	void							prepare(const Mesh* mesh);

		
			
	void							triangulatePolygonWithEarClipping(std::vector<uint32_t>& inputPolygon, Vertex* vert, ProjectionDirections dir);
	void							buildPolygonAndTriangulate(std::vector<Edge>& edges, Vertex* vertices, int32_t userData, int32_t materialId, int32_t smoothingGroup);
	void							computePositionedMapping();
	
	std::vector<TriangleIndexed>						mBaseMeshTriangles;	
	/**
		Final triangles
	*/
	std::vector<Triangle>								mBaseMeshResultTriangles;
	std::vector<Triangle>								mBaseMeshUVFittedTriangles;
};

} // namespace Blast
} // namespace Nv


#endif	// ifndef NVBLASTEXTAUTHORINGTRIANGULATOR_H
