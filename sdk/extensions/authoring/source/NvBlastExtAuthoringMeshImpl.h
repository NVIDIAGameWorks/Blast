/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTAUTHORINGMESHIMPL_H
#define NVBLASTAUTHORINGMESHIMPL_H

#include "NvBlastExtAuthoringMesh.h"
#include <vector>

namespace Nv
{
namespace Blast
{

/**
	Class for internal mesh representation
*/
class MeshImpl : public Mesh
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
	MeshImpl(const physx::PxVec3* position, const physx::PxVec3* normals, const physx::PxVec2* uv, uint32_t verticesCount, const uint32_t* indices, uint32_t indicesCount);

	/**
		Constructs mesh object from array of facets.
		\param[in] vertices		Array of vertices
		\param[in] edges		Array of edges
		\param[in] facets		Array of facets
		\param[in] posCount		Vertices count
		\param[in] edgesCount	Edges count
		\param[in] facetsCount	Facets count
	*/
	MeshImpl(const Vertex* vertices, const Edge* edges, const Facet* facets, uint32_t posCount, uint32_t edgesCount, uint32_t facetsCount);

	~MeshImpl();

	virtual void		release() override;

	/**
		Return true if mesh is valid
	*/
	bool				isValid() const override;

	/**
		Return pointer on vertices array
	*/
	Vertex*				getVerticesWritable() override;

	/**
		Return pointer on edges array
	*/
	Edge*				getEdgesWritable() override;

	/**
		Return pointer on facets array
	*/
	Facet*				getFacetsBufferWritable() override;

	/**
	Return pointer on vertices array
	*/
	const Vertex*			getVertices() const override;

	/**
	Return pointer on edges array
	*/
	const Edge*				getEdges() const override;

	/**
	Return pointer on facets array
	*/
	const Facet*			getFacetsBuffer() const override;

	/**
		Return writable pointer on specified facet
	*/
	Facet*				getFacetWritable(int32_t facet) override;

	/**
	Return writable pointer on specified facet
	*/
	const Facet*		getFacet(int32_t facet) const override;

	/**
		Return edges count
	*/
	uint32_t			getEdgesCount() const override;

	/**
		Return vertices count
	*/
	uint32_t			getVerticesCount() const override;

	/**
		Return facet count
	*/
	uint32_t			getFacetCount() const override;


	/**
		Return reference on mesh bounding box.
	*/
	const physx::PxBounds3&	getBoundingBox() const override;

	/**
		Return writable reference on mesh bounding box.
	*/
	physx::PxBounds3&	getBoundingBoxWritable() override;

	/**
		Recalculate bounding box
	*/
	void				recalculateBoundingBox() override;

	/**
		Compute mesh volume. Can be used only for triangulated meshes.
		Return mesh volume. If mesh is not triangulated return 0.
	*/
	float				getMeshVolume() override;


	/**
	Set per-facet material id.
	*/
	void	setMaterialId(const int32_t* materialIds) override;

	/**
	Replaces an material id on faces with a new one
	*/
	void	replaceMaterialId(int32_t oldMaterialId, int32_t newMaterialId) override;

	/**
	Set per-facet smoothing group.
	*/
	void	setSmoothingGroup(const int32_t* smoothingGroups) override;

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
Mesh*	getCuttingBox(const physx::PxVec3& point, const physx::PxVec3& normal, float size, int32_t id, int32_t interiorMaterialId);

/**
	Create box at some particular position.
	\param[in] point	Cutting face center
	\param[in] size		Cutting box size
*/
Mesh*	getBigBox(const physx::PxVec3& point, float size, int32_t interiorMaterialId);

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
Mesh* getNoisyCuttingBoxPair(const physx::PxVec3& point, const physx::PxVec3& normal, float size, float jaggedPlaneSize, uint32_t resolution, int32_t id, float amplitude, float frequency, int32_t octaves, int32_t seed, int32_t interiorMaterialId);


/**
	Inverses normals of cutting box and sets indices. 
	\param[in] mesh		Cutting box mesh
	\param[in] id	Cutting box ID
*/
void inverseNormalAndSetIndices(Mesh* mesh, int32_t id);

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTAUTHORINGMESHIMPL_H
