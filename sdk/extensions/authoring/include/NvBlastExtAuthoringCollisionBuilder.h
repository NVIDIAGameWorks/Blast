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


#ifndef NVBLASTEXTAUTHORINGCOLLISIONBUILDER_H
#define NVBLASTEXTAUTHORINGCOLLISIONBUILDER_H

#include "NvBlastTypes.h"

namespace physx
{
class PxCooking;
class PxPhysicsInsertionCallback;
class PxVec3;
class PxConvexMesh;
}


namespace Nv
{
namespace Blast
{

struct CollisionHull;
struct Triangle;

struct CollisionParams
{
	CollisionParams()
	{
		setDefault();
	}
	void setDefault()
	{
		maximumNumberOfHulls = 8;
		maximumNumberOfVerticesPerHull = 64;
		voxelGridResolution = 1000000;
		concavity = 0.0025f;
	}
	uint32_t maximumNumberOfHulls; // Maximum number of convex hull generated for one chunk. If equal to 1 convex decomposition is disabled.
	uint32_t maximumNumberOfVerticesPerHull; // Controls the maximum number of triangles per convex-hull (default=64, range=4-1024)
	uint32_t voxelGridResolution; // Voxel grid resolution used for chunk convex decomposition (default=1,000,000, range=10,000-16,000,000).
	float concavity; // Value between 0 and 1, controls how accurate hull generation is
};

/**
	ConvexMeshBuilder provides routine to build collision hulls from array of vertices.
	Collision hull is built as convex hull of provided point set.
	If due to some reason building of convex hull is failed, collision hull is built as bounding box of vertex set.
*/
class ConvexMeshBuilder
{
public:
	virtual ~ConvexMeshBuilder() {}

	/**
	Release ConvexMeshBuilder memory
	*/
	virtual void					release() = 0;

	/**
		Method creates CollisionHull from provided array of vertices.
		\param[in]  verticesCount	Number of vertices
		\param[in]	vertexData		Vertex array of some object, for which collision geometry should be built
		\param[out] output			Reference on CollisionHull object in which generated geometry should be saved
	*/
	virtual CollisionHull*			buildCollisionGeometry(uint32_t verticesCount, const physx::PxVec3* vertexData) = 0;

	/**
	Method creates PxConvexMesh from provided array of vertices.
	\param[in]  verticesCount		Number of vertices
	\param[in]	vertexData			Vertex array of some object, for which collision geometry should be built

	\return pointer to the PxConvexMesh object if it was built successfully, 'nullptr' otherwise.
	*/
	virtual physx::PxConvexMesh*	buildConvexMesh(uint32_t verticesCount, const physx::PxVec3* vertexData) = 0;


	/**
		Method creates PxConvexMesh from provided ConvexHull geometry
		\param[in]	hull			ConvexHull geometry

		\return pointer to the PxConvexMesh object if it was built successfully, 'nullptr' otherwise.
	*/
	virtual physx::PxConvexMesh*	buildConvexMesh(const CollisionHull& hull) = 0;


	/**
		Convex geometry trimming. 
		Using slicing with noised slicing surface can result in intersecting collision geometry.
		It leads to unstable behaviour of rigid body simulation. 
		This method trims all intersecting parts of collision geometry.
		As a drawback, trimming collision geometry can lead to penetrating render meshes during simulation.

		\param[in]		chunksCount	Number of chunks
		\param[in,out]	in			ConvexHull geometry which should be clipped. 
		\param[in]		chunkDepth	Array of depth levels of convex hulls corresponding chunks.

	*/
	virtual void					trimCollisionGeometry(uint32_t chunksCount, CollisionHull** in, const uint32_t* chunkDepth) = 0;


	/**
		Create mesh convex decomposition
	*/
	virtual int32_t					buildMeshConvexDecomposition(const Nv::Blast::Triangle* mesh, uint32_t triangleCount, const CollisionParams& params, CollisionHull** &convexes) = 0;

};

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTAUTHORINGCOLLISIONBUILDER_H
