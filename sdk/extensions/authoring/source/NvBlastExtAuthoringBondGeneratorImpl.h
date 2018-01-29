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
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.


#ifndef NVBLASTEXTAUTHORINGBONDGENERATORIMPL_H
#define NVBLASTEXTAUTHORINGBONDGENERATORIMPL_H

#include "NvBlastExtAuthoringBondGenerator.h"
#include "NvBlastExtAuthoringFractureTool.h"
#include "../cooking/PxCooking.h"
#include <PxPlane.h>
#include <NvBlastExtAuthoringCollisionBuilder.h>
#include <vector>

namespace Nv
{
namespace Blast
{

/**
	Tool for gathering bond information from provided mesh geometry
*/

class BlastBondGeneratorImpl : public BlastBondGenerator
{
public:
				
	BlastBondGeneratorImpl(physx::PxCooking* cooking, physx::PxPhysicsInsertionCallback* insertionCallback) 
		: mPxCooking(cooking), mPxInsertionCallback(insertionCallback){};

	virtual void release() override;

	virtual int32_t	buildDescFromInternalFracture(FractureTool* tool, const bool* chunkIsSupport,
		NvBlastBondDesc*& resultBondDescs, NvBlastChunkDesc*& resultChunkDescriptors)  override;

	virtual int32_t	createBondBetweenMeshes(uint32_t meshACount, const Triangle* meshA, uint32_t meshBCount, const Triangle* meshB,
		NvBlastBond& resultBond, BondGenerationConfig conf = BondGenerationConfig()) override;

	virtual int32_t	createBondBetweenMeshes(uint32_t meshCount, const uint32_t* geometryOffset, const Triangle* geometry,
		uint32_t overlapsCount, const uint32_t* overlapsA, const uint32_t* overlapsB,
		NvBlastBondDesc*& resultBond, BondGenerationConfig cfg) override;

	virtual int32_t	bondsFromPrefractured(uint32_t meshCount, const uint32_t* geometryOffset, const Triangle* geometry,
		const bool* chunkIsSupport, NvBlastBondDesc*& resultBondDescs,
		BondGenerationConfig conf = BondGenerationConfig()) override;

	virtual int32_t	bondsFromPrefractured(uint32_t meshCount, const uint32_t* convexHullOffset, const CollisionHull** chunkHulls,
		const bool* chunkIsSupport, const uint32_t* meshGroups, NvBlastBondDesc*& resultBondDescs, float maxSeparation) override;
				
private:
	float	processWithMidplanes(	TriangleProcessor* trProcessor, 
									const std::vector<physx::PxVec3>& chunk1Points, const std::vector<physx::PxVec3>& chunk2Points, 
									const std::vector<physx::PxVec3>& hull1p, const std::vector<physx::PxVec3>& hull2p, 
									physx::PxVec3& normal, physx::PxVec3& centroid, float maxSeparation);

	int32_t	createFullBondListAveraged(	uint32_t meshCount, const uint32_t* geometryOffset, const Triangle* geometry, const CollisionHull** chunkHulls,
										const bool* supportFlags, const uint32_t* meshGroups, NvBlastBondDesc*& resultBondDescs, BondGenerationConfig conf);
	int32_t	createFullBondListExact(	uint32_t meshCount, const uint32_t* geometryOffset, const Triangle* geometry,
										const bool* supportFlags, NvBlastBondDesc*& resultBondDescs, BondGenerationConfig conf);
	int32_t	createFullBondListExactInternal(uint32_t meshCount, const uint32_t* geometryOffset, const Triangle* geometry,
											std::vector<PlaneChunkIndexer>& planeTriangleMapping , NvBlastBondDesc*& resultBondDescs);
	int32_t	createBondForcedInternal(	const std::vector<physx::PxVec3>& hull0, const std::vector<physx::PxVec3>& hull1,const CollisionHull& cHull0, 
										const CollisionHull& cHull1, physx::PxBounds3 bound0, physx::PxBounds3 bound1, NvBlastBond& resultBond, float overlapping);

	void	buildGeometryCache(uint32_t meshCount, const uint32_t* geometryOffset, const Triangle* geometry);
	void	resetGeometryCache();

	physx::PxCooking*							mPxCooking;
	physx::PxPhysicsInsertionCallback*			mPxInsertionCallback;


	std::vector<std::vector<Triangle> >			mGeometryCache;

	std::vector<PlaneChunkIndexer>				mPlaneCache;
	std::vector<CollisionHull*>					mCHullCache;
	std::vector<std::vector<physx::PxVec3> >	mHullsPointsCache;
	std::vector<physx::PxBounds3 >				mBoundsCache;
};

}	// namespace Blast
}	// namespace Nv

#endif // NVBLASTEXTAUTHORINGBONDGENERATORIMPL_H