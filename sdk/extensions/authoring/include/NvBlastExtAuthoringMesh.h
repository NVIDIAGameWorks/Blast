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


#ifndef NVBLASTAUTHORINGMESH_H
#define NVBLASTAUTHORINGMESH_H

#include "NvBlastExtAuthoringTypes.h"

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
	virtual ~Mesh() {}

	/**
		Release Mesh memory
	*/
	virtual void				release() = 0;

	/**
		Return true if mesh is valid
	*/
	virtual bool				isValid() const = 0;

	/**
		Return writable pointer on vertices array
	*/
	virtual Vertex*				getVerticesWritable() = 0;

	/**
	Return pointer on vertices array
	*/
	virtual const Vertex*		getVertices() const = 0;


	/**
		Return writable pointer on edges array
	*/
	virtual Edge*				getEdgesWritable() = 0;

	/**
	Return pointer on edges array
	*/
	virtual const Edge*			getEdges() const = 0;

	/**
		Return writable pointer on facets array
	*/
	virtual Facet*				getFacetsBufferWritable() = 0;

	/**
	Return pointer on facets array
	*/
	virtual const Facet*		getFacetsBuffer() const = 0;

	/**
		Return writable pointer on specified facet
	*/
	virtual Facet*				getFacetWritable(int32_t facet) = 0;
	/**
		Return pointer on specified facet
	*/
	virtual const Facet*		getFacet(int32_t facet) const = 0;

	/**
		Return edges count
	*/
	virtual uint32_t			getEdgesCount() const = 0;

	/**
		Return vertices count
	*/
	virtual uint32_t			getVerticesCount() const = 0;

	/**
		Return facet count
	*/
	virtual uint32_t			getFacetCount() const = 0;

	/**
		Return reference on mesh bounding box.
	*/
	virtual const physx::PxBounds3&	getBoundingBox() const = 0;

	/**
		Return writable reference on mesh bounding box.
	*/
	virtual physx::PxBounds3&	getBoundingBoxWritable() = 0;


	/**
		Set per-facet material id.
	*/
	virtual void	setMaterialId(const int32_t* materialIds) = 0;

	/**
	Replaces an material id on faces with a new one
	*/
	virtual void	replaceMaterialId(int32_t oldMaterialId, int32_t newMaterialId) = 0;

	/**
		Set per-facet smoothing group.
	*/
	virtual void	setSmoothingGroup(const int32_t* smoothingGroups) = 0;

	/**
		Recalculate bounding box
	*/
	virtual void				recalculateBoundingBox() = 0;

	/**
		Compute mesh volume. Can be used only for triangulated meshes.
		Return mesh volume. If mesh is not triangulated return 0.
	*/
	virtual float				getMeshVolume() = 0;
};

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTAUTHORINGMESH_H
