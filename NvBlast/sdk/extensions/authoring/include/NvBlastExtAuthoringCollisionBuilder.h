/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTEXTAUTHORINGCOLLISIONBUILDER_H
#define NVBLASTEXTAUTHORINGCOLLISIONBUILDER_H

#include "NvBlastTypes.h"
#include <vector>
#include <PxVec3.h>

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

	CollisionHull(){};

	std::vector<physx::PxVec3>	points;
	std::vector<uint32_t>		indices;
	std::vector<HullPolygon>	polygonData;
};


/**
	ConvexMeshBuilder provides routine to build collision hulls from array of vertices.
	Collision hull is built as convex hull of provided point set.
	If due to some reason building of convex hull is failed, collision hull is built as bounding box of vertex set.
*/
class ConvexMeshBuilder
{
public:

	/**
		Constructor should be provided with PxCoocking and PxPhysicsInsertionCallback objects.
	*/
	ConvexMeshBuilder(physx::PxCooking* cooking, physx::PxPhysicsInsertionCallback* insertionCallback) : mInsertionCallback(insertionCallback), mCooking(cooking) {}

	/**
		Method creates CollisionHull from provided array of vertices.
		\param[in]	vertexData		Vertex array of some object, for which collision geometry should be built
		\param[out] output			Reference on CollisionHull object in which generated geometry should be saved
	*/
	void					buildCollisionGeometry(const std::vector<physx::PxVec3>& vertexData, CollisionHull& output);

	/**
	Method creates PxConvexMesh from provided array of vertices.
	\param[in]	vertexData		Vertex array of some object, for which collision geometry should be built

	\return pointer to the PxConvexMesh object if it was built successfully, 'nullptr' otherwise.
	*/
	physx::PxConvexMesh*	buildConvexMesh(std::vector<physx::PxVec3>& vertexData);


	/**
		Method creates PxConvexMesh from provided ConvexHull geometry
		\param[in]	hull ConvexHull geometry

		\return pointer to the PxConvexMesh object if it was built successfully, 'nullptr' otherwise.
	*/
	physx::PxConvexMesh*	buildConvexMesh(CollisionHull& hull);


	/**
		Convex geometry trimming. 
		Using slicing with noised slicing surface can result in intersecting collision geometry.
		It leads to unstable behaviour of rigid body simulation. 
		This method trims all intersecting parts of collision geometry.
		As a drawback, trimming collision geometry can lead to penetrating render meshes during simulation.


		\param[in]	in ConvexHull geometry which should be clipped. 
		\param[in]	chunkDepth Array of depth levels of convex hulls corresponding chunks.

	*/
	
	void					trimCollisionGeometry(std::vector<CollisionHull>& in, const std::vector<uint32_t>& chunkDepth);


private:
	physx::PxPhysicsInsertionCallback*	mInsertionCallback;
	physx::PxCooking*					mCooking;
};

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTAUTHORINGCOLLISIONBUILDER_H
