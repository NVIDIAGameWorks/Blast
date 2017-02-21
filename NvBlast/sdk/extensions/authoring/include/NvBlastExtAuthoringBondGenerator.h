/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTEXTAUTHORINGBONDGENERATOR_H
#define NVBLASTEXTAUTHORINGBONDGENERATOR_H

#include "NvBlastExtAuthoringTypes.h"
#include "NvBlastExtAuthoringFractureTool.h"
#include "NvBlastTypes.h"
#include "../cooking/PxCooking.h"
#include <PxPlane.h>
#include <NvBlastExtAuthoringCollisionBuilder.h>
struct NvBlastBondDesc;
struct NvBlastChunkDesc;
struct NvBlastBond;

using namespace physx;


namespace Nv
{
namespace Blast
{

// Forward declarations
class TriangleProcessor;
struct PlaneChunkIndexer;

/**
	Bond interface generation configuration
	EXACT - common surface will be searched
	AVERAGE - Inerface is approximated by projections or intersecitons with midplane
*/
struct BondGenerationConfig
{
	enum BondGenMode { EXACT, AVERAGE };
	BondGenMode bondMode;
};


struct PlaneChunkIndexer
{
	PlaneChunkIndexer(int32_t chunkId, int32_t trId, physx::PxPlane pl) : chunkId(chunkId), trId(trId), plane(pl) {}
	int32_t chunkId;
	int32_t trId;
	physx::PxPlane plane;
};


/**
	Tool for gathering bond information from provided mesh geometry
*/

class BlastBondGenerator
{
public:
				
	BlastBondGenerator(physx::PxCooking* cooking, physx::PxPhysicsInsertionCallback* insertionCallback) : mPxCooking(cooking), mPxInsertionCallback(insertionCallback){};

	/**
		This method based on marking triangles during fracture process, so can be used only with internally fractured meshes.
		\param[in] tool				FractureTool which contains chunks representation, tool->finalizeFracturing() should be called before.
		\param[in] chunkIsSupport	Array of flags, if true - chunk is support. Array size should be equal to chunk count in tool.
		\param[out] resultBondDescs	Array of created bond descriptors.
		\param[out] resultChunkDescriptors	Array of created chunk descriptors.
		\return 0 if success
	*/
	int32_t	buildDescFromInternalFracture(FractureTool* tool, const std::vector<bool>& chunkIsSupport, std::vector<NvBlastBondDesc>& resultBondDescs, std::vector<NvBlastChunkDesc>& resultChunkDescriptors);


	/**
		Creates bond description between two meshes
		\param[in] meshA	Array of triangles of mesh A.
		\param[in] meshB	Array of triangles of mesh B.
		\param[out] resultBond	Result bond description.
		\param[in] conf	Bond creation mode.
		\return 0 if success
	*/
	int32_t	createBondBetweenMeshes(const std::vector<Triangle>& meshA, const std::vector<Triangle>& meshB, NvBlastBond& resultBond, BondGenerationConfig conf = BondGenerationConfig());

	/**
		Creates bond description between number of meshes
		\param[in] geometry	Array of arrays of triangles for each chunk.
		\param[out] resultBond	Array of result bonds.
		\param[in] overlaps	Array of pairs - indexes of chunks, for which bond should be created.
		\param[in] cfg	Bond creation mode.
		\return 0 if success
	*/
	int32_t	createBondBetweenMeshes(const std::vector<std::vector<Triangle> >& geometry, std::vector<NvBlastBondDesc>& resultBond, const std::vector<std::pair<uint32_t, uint32_t> >& overlaps, BondGenerationConfig cfg);


	/**
		Creates bond description for prefractured meshes, when there is no info about which chunks should be connected with bond.
		\param[in] geometry	Array of arrays of triangles for each chunk.
		\param[in] chunkIsSupport Array of flags, if true - chunk is support. Array size should be equal to chunk count in tool.
		\param[out] resultBondDescs	Array of result bonds.
		\param[in] conf	Bond creation mode.
		\return 0 if success
	*/
	int32_t	bondsFromPrefractured(const std::vector<std::vector<Triangle>>& geometry, const std::vector<bool>& chunkIsSupport, std::vector<NvBlastBondDesc>& resultBondDescs, BondGenerationConfig conf = BondGenerationConfig());
				
private:
	float	processWithMidplanes(TriangleProcessor* trProcessor, const std::vector<physx::PxVec3>& chunk1Points, const std::vector<physx::PxVec3>& chunk2Points,
								 const std::vector<physx::PxVec3>& hull1p,const std::vector<physx::PxVec3>& hull2p, physx::PxVec3& normal, physx::PxVec3& centroid);

	int32_t	createFullBondListAveraged(const std::vector<std::vector<Triangle>>& chunksGeometry, const std::vector<bool>& supportFlags, std::vector<NvBlastBondDesc>& mResultBondDescs, BondGenerationConfig conf);
	int32_t	createFullBondListExact(const std::vector<std::vector<Triangle>>& chunksGeometry, const std::vector<bool>& supportFlags, std::vector<NvBlastBondDesc>& mResultBondDescs, BondGenerationConfig conf);
	int32_t	createFullBondListExactInternal(const std::vector<std::vector<Triangle>>& chunksGeometry, std::vector < PlaneChunkIndexer >& planeTriangleMapping , std::vector<NvBlastBondDesc>& mResultBondDescs);
	int32_t	createBondForcedInternal(const std::vector<PxVec3>& hull0, const std::vector<PxVec3>& hull1,const CollisionHull& cHull0, const CollisionHull& cHull1,PxBounds3 bound0, PxBounds3 bound1, NvBlastBond& resultBond, float overlapping);

	void	buildGeometryCache(const std::vector<std::vector<Triangle> >& geometry);
	void	resetGeometryCache();

	physx::PxCooking*							mPxCooking;
	physx::PxPhysicsInsertionCallback*			mPxInsertionCallback;


	std::vector<std::vector<Triangle> >			mGeometryCache;

	std::vector<PlaneChunkIndexer>				mPlaneCache;
	std::vector<CollisionHull>					mCHullCache;
	std::vector<std::vector<physx::PxVec3> >	mHullsPointsCache;
	std::vector<physx::PxBounds3 >				mBoundsCache;
};

}	// namespace Blast
}	// namespace Nv

#endif // NVBLASTEXTAUTHORINGBONDGENERATOR_H