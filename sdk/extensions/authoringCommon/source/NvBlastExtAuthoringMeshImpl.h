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

#ifndef NVBLASTAUTHORINGMESHIMPL_H
#define NVBLASTAUTHORINGMESHIMPL_H

#include "NvBlastExtAuthoringMesh.h"
#include <PxBounds3.h>
#include <vector>
#include <map>
#include <set>

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
	MeshImpl(const NvcVec3* position, const NvcVec3* normals, const NvcVec2* uv, uint32_t verticesCount, const uint32_t* indices, uint32_t indicesCount);

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

	MeshImpl(const Vertex* vertices, uint32_t count);

	MeshImpl(const Vertex* vertices, uint32_t count, uint32_t* indices, uint32_t indexCount, void* materials, uint32_t materialStride);

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
	const NvcBounds3&	getBoundingBox() const override;

	/**
		Return writable reference on mesh bounding box.
	*/
	NvcBounds3&	getBoundingBoxWritable() override;

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

	/**
		Calculate per-facet bounding boxes.
	*/
	virtual void						calcPerFacetBounds() override;

	/**
		Get pointer on facet bounding box, if not calculated return nullptr.
	*/
	virtual const  NvcBounds3*	getFacetBound(uint32_t index) const override;

private:
	std::vector<Vertex>	mVertices;
	std::vector<Edge>	mEdges;
	std::vector<Facet>	mFacets;
	physx::PxBounds3 mBounds;
	std::vector<physx::PxBounds3> mPerFacetBounds;
};

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTAUTHORINGMESHIMPL_H
