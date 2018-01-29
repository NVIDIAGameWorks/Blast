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


#ifndef NVBLASTEXTAUTHORINGCOLLISIONBUILDERIIMPL_H
#define NVBLASTEXTAUTHORINGCOLLISIONBUILDERIIMPL_H

#include "NvBlastExtAuthoringCollisionBuilder.h"
#include "NvBlastExtAuthoringTypes.h"

namespace Nv
{
namespace Blast
{

struct CollisionHullImpl : public CollisionHull
{
	~CollisionHullImpl();
	CollisionHullImpl()
	{
		pointsCount = 0;
		indicesCount = 0;
		polygonDataCount = 0;
		points = nullptr;
		indices = nullptr;
		polygonData = nullptr;
	}

	CollisionHullImpl(const CollisionHull& hullToCopy);

	void release() override;
};
	
class ConvexMeshBuilderImpl : public ConvexMeshBuilder
{
public:

	/**
		Constructor should be provided with PxCoocking and PxPhysicsInsertionCallback objects.
	*/
	ConvexMeshBuilderImpl(physx::PxCooking* cooking, physx::PxPhysicsInsertionCallback* insertionCallback) : mInsertionCallback(insertionCallback), mCooking(cooking) {}

	virtual void					release() override;

	virtual CollisionHull*			buildCollisionGeometry(uint32_t verticesCount, const physx::PxVec3* vertexData) override;

	virtual physx::PxConvexMesh*	buildConvexMesh(uint32_t verticesCount, const physx::PxVec3* vertexData) override;

	virtual physx::PxConvexMesh*	buildConvexMesh(const CollisionHull& hull) override;
	
	virtual void					trimCollisionGeometry(uint32_t chunksCount, CollisionHull** in, const uint32_t* chunkDepth) override;

	virtual int32_t					buildMeshConvexDecomposition(const Triangle* mesh, uint32_t triangleCount, const CollisionParams& params, CollisionHull**& convexes) override;

private:
	physx::PxPhysicsInsertionCallback*	mInsertionCallback;
	physx::PxCooking*					mCooking;
};

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTAUTHORINGCOLLISIONBUILDERIIMPL_H
