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


#ifndef NVBLASTEXTPXCOLLISIONBUILDER_H
#define NVBLASTEXTPXCOLLISIONBUILDER_H

#include "NvBlastExtAuthoringConvexMeshBuilder.h"

namespace physx
{
	class PxConvexMesh;
}
namespace Nv
{
namespace Blast
{
struct AuthoringResult;
struct ExtPxChunk;
struct ExtPxSubchunk;

/**
ConvexMeshBuilder provides routine to build collision hulls from array of vertices.
Collision hull is built as convex hull of provided point set.
If due to some reason building of convex hull is failed, collision hull is built as bounding box of vertex set.
PhysX implementation can be found in NvBlastExtPx.
*/
class ExtPxCollisionBuilder : public ConvexMeshBuilder
{
  public:
	/**
	Method creates user defined collision mesh from provided array of vertices.
	ConvexMeshBuilder from ExtPhysX returns PxConvexMesh pointer.
	\param[in]  hull	Collision hull.
	*/
	virtual physx::PxConvexMesh* buildConvexMesh(const CollisionHull& hull) = 0;

	/**
	Build physics chunks and subchunks from collision hulls
	*/
	virtual void buildPhysicsChunks(uint32_t chunkCount, uint32_t* hullOffsets, CollisionHull** hulls,
	                                ExtPxChunk* physicsChunks, ExtPxSubchunk* physicsSubchunks) = 0;
};

}  // namespace Blast
}  // namespace Nv


#endif  // ifndef NVBLASTEXTPXCOLLISIONBUILDER_H
