/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

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
class SimplexNoise;

/**
	Vertex comparator for vertex welding.
*/
struct VrtComp
{
	bool operator()(const Vertex& a, const Vertex& b) const;
};

/**
	Vertex comparator for vertex welding (not accounts normal and uv parameters of vertice).
*/
struct VrtPositionComparator
{
	bool operator()(const physx::PxVec3& a, const physx::PxVec3& b) const;
};

/**
	Structure used on tesselation stage. Maps edge to two neighboor triangles
*/
struct EdgeToTriangles
{
	int32_t tr[2];
	int32_t c;
	EdgeToTriangles()
	{
		c = 0;
	}
	/**
		Add triangle to edge. Should not be called more than twice for one edge!!!!.
	*/
	void	add(int32_t t)
	{
		tr[c] = t;
		++c;
	}
	/**
		Replaces mapping from one triangle to another.
	*/
	void	replace(int32_t from, int32_t to)
	{
		if (tr[0] == from)
		{
			tr[0] = to;
		}
		else
		{
			if (c == 2 && tr[1] == from)
			{
				tr[1] = to;
			}
		}
	}
	/**
		Get triangle which is mapped by this edge and which index is different than provided.
	*/
	int32_t	getNot(int32_t id)
	{
		if (tr[0] != id)
		{
			return tr[0];
		}
		if (c == 2 && tr[1] != id)
		{
			return tr[1];
		}
		return -1;
	}

};


/**
	Tool for doing all post processing steps of authoring.
*/
class ChunkPostProcessor
{
public:
	/**
		Edge flags
	*/
	enum EdgeFlag{ INTERNAL_EDGE, EXTERNAL_BORDER_EDGE, INTERNAL_BORDER_EDGE, EXTERNAL_EDGE, NONE };

	/**
		Triangulates provided mesh and saves result internally. Uses Ear-clipping algorithm.
		\param[in] mesh Mesh for triangulation
	*/
	void							triangulate(Mesh* mesh);

	/**
		\return Return array of triangles of base mesh.
	*/
	std::vector<Triangle>&			getBaseMesh()
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
		Tesselate internal surface. 
		\param[in] maxLen - maximal length of edge on internal surface. 
	*/
	void							tesselateInternalSurface(float maxLen);

	/**
		Apply noise to internal surface. Must be called only after tesselation!!! 
		\param[in] noise - noise generator
		\param[in] falloff - damping of noise around of external surface
		\param[in] relaxIterations - number of smoothing iterations before applying noise
		\param[in] relaxFactor - amount of smooting before applying noise.
	*/
	void							applyNoise(SimplexNoise& noise, float falloff, int32_t relaxIterations, float relaxFactor);

	/**
		\return Return array of noised mesh triangles.
	*/
	std::vector<Triangle>&			getNoisyMesh()
	{
		return mTesselatedMeshResultTriangles;
	};

	/**
		Removes all information about mesh triangulation, tesselation, etc.
	*/
	void							reset();

private:

	

	void							collapseEdge(int32_t id);
	void							divideEdge(int32_t id);
	void							updateVertEdgeInfo();
	void							updateEdgeTriangleInfo();

	int32_t							addVerticeIfNotExist(Vertex& p);
	void							addEdgeIfValid(EdgeWithParent& ed);
	
	/* Data used before triangulation to build polygon loops*/

	std::vector<Vertex>									mVertices;
	std::vector<EdgeWithParent>							mBaseMeshEdges;
	std::map<Vertex, int32_t, VrtComp>					mVertMap;
	std::map<EdgeWithParent, int32_t, EdgeComparator>	mEdgeMap;
	std::vector<uint32_t>								mBaseMapping;
	


	/**
		Unite all almost similar vertices, update edges according to this changes
	*/
	void							prepare(Mesh* mesh);

	/* ------------------------------------------------------------ */

	/* Triangulation and tesselation stage data */
	bool												isTesselated;

	std::map<Edge,	int32_t>							mTrMeshEdgeMap;
	std::vector<Edge>									mTrMeshEdges;
	std::vector<EdgeToTriangles>						mTrMeshEdToTr;
	std::vector<int32_t>								mVertexValence;
	std::vector<std::vector<int32_t> >					mVertexToTriangleMap;

	std::vector<bool>									mRestrictionFlag;
	std::vector<EdgeFlag>								mEdgeFlag;

	std::vector<float>									mVerticesDistances;
	std::vector<physx::PxVec3>							mVerticesNormalsSmoothed;
	std::vector<int32_t>								mPositionMappedVrt;
	std::vector<std::vector<int32_t> >					mGeometryGraph;


	int32_t							addEdgeTr(const Edge& ed);
	int32_t							findEdge(const Edge& e);

	void							prebuildEdgeFlagArray();
	void							computePositionedMapping();
	
	void							computeFalloffAndNormals();


		
	void							triangulatePolygonWithEarClipping(std::vector<uint32_t>& inputPolygon, Vertex* vert, ProjectionDirections dir);
	void							buildPolygonAndTriangulate(std::vector<Edge>& edges, Vertex* vertices, int32_t userData);
	
	void							relax(int32_t iterations, float factor, std::vector<Vertex>& vertices);
	void							recalcNoiseDirs();

	std::vector<TriangleIndexed>						mBaseMeshTriangles;
	std::vector<TriangleIndexed>						mTesselatedMeshTriangles;

	/**
		Final triangles
	*/
	void prebuildTesselatedTriangles();

	std::vector<Triangle>								mBaseMeshResultTriangles;
	std::vector<Triangle>								mTesselatedMeshResultTriangles;
};

} // namespace Blast
} // namespace Nv


#endif	// ifndef NVBLASTEXTAUTHORINGTRIANGULATOR_H
