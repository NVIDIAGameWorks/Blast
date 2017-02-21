/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTAUTHORINGMESH_H
#define NVBLASTAUTHORINGMESH_H

#include "NvBlastExtAuthoringTypes.h"
#include <vector>


namespace Nv
{
namespace Blast
{

/**
	Class for internal mesh representation
*/
class Mesh
{
public:

	/**
		Constructs mesh object from array of triangles.
		\param[in] position			Array of vertex positions
		\param[in] normals			Array of vertex normals
		\param[in] uv				Array of vertex uv coordinates
		\param[in] verticesCount	Vertices count
		\param[in] indices			Array of vertex indices. Indices contain vertex index triplets which form a mesh triangle. 
		\param[in] indicesCount		Indices count (should be equal to numberOfTriangles * 3)
	*/
	Mesh(physx::PxVec3* position, physx::PxVec3* normals, physx::PxVec2* uv, uint32_t verticesCount, uint32_t* indices, uint32_t indicesCount);

	/**
		Constructs mesh object from array of facets.
		\param[in] vertices		Array of vertices
		\param[in] edges		Array of edges
		\param[in] facets		Array of facets
		\param[in] posCount		Vertices count
		\param[in] edgesCount	Edges count
		\param[in] facetsCount	Facets count
	*/
	Mesh(Vertex* vertices, Edge* edges, Facet* facets, uint32_t posCount, uint32_t edgesCount, uint32_t facetsCount);

	~Mesh();

	/**
		Return true if mesh is valid
	*/
	bool				isValid();

	/**
		Return pointer on vertices array
	*/
	Vertex*				getVertices();

	/**
		Return pointer on edges array
	*/
	Edge*				getEdges();

	/**
		Return pointer on facets array
	*/
	Facet*				getFacetsBuffer();

	/**
		Return pointer on specified facet
	*/
	Facet*				getFacet(int32_t facet);

	/**
		Return edges count
	*/
	uint32_t			getEdgesCount();

	/**
		Return vertices count
	*/
	uint32_t			getVerticesCount();

	/**
		Return facet count
	*/
	uint32_t			getFacetCount();

	/**
		Return reference on mesh bounding box.
	*/
	physx::PxBounds3&	getBoundingBox();

	/**
		Recalculate bounding box
	*/
	void				recalculateBoundingBox();

	/**
		Compute mesh volume. Can be used only for triangulated meshes.
		Return mesh volume. If mesh is not triangulated return 0.
	*/
	float				getMeshVolume();

private:
	std::vector<Vertex>	mVertices;
	std::vector<Edge>	mEdges;
	std::vector<Facet>	mFacets;
	physx::PxBounds3	mBounds;
};


/**
	Helper functions
*/

/**
	Set cutting box at some particular position.
	\param[in] point	Cutting face center
	\param[in] normal	Cutting face normal
	\param[in] mesh		Cutting box mesh
	\param[in] size		Cutting box size
	\param[in] id	Cutting box ID
*/
void	setCuttingBox(const physx::PxVec3& point, const physx::PxVec3& normal, Mesh* mesh, float size, int32_t id);
/**
	Create cutting box at some particular position.
	\param[in] point	Cutting face center
	\param[in] normal	Cutting face normal
	\param[in] size		Cutting box size
	\param[in] id	Cutting box ID
*/
Mesh*	getCuttingBox(const physx::PxVec3& point, const physx::PxVec3& normal, float size, int32_t id);

/**
	Create box at some particular position.
	\param[in] point	Cutting face center
	\param[in] size		Cutting box size
*/
Mesh*	getBigBox(const physx::PxVec3& point, float size);

/**
	Create slicing box with noisy cutting surface.
	\param[in] point			Cutting face center
	\param[in] normal			Cutting face normal
	\param[in] size				Cutting box size
	\param[in] jaggedPlaneSize	Noisy surface size
	\param[in] resolution		Noisy surface resolution
	\param[in] id				Cutting box ID
	\param[in] amplitude		Noise amplitude
	\param[in] frequency		Noise frequency
	\param[in] octaves			Noise octaves
	\param[in] seed				Random generator seed, used for noise generation.	
*/
Mesh* getNoisyCuttingBoxPair(const physx::PxVec3& point, const physx::PxVec3& normal, float size, float jaggedPlaneSize, uint32_t resolution, int32_t id, float amplitude, float frequency, int32_t octaves, int32_t seed);


/**
	Inverses normals of cutting box and sets indices. 
	\param[in] mesh		Cutting box mesh
	\param[in] id	Cutting box ID
*/
void inverseNormalAndSetIndices(Mesh* mesh, int32_t id);

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTAUTHORINGMESH_H
