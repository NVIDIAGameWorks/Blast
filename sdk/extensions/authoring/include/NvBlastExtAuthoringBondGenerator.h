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


#ifndef NVBLASTEXTAUTHORINGBONDGENERATOR_H
#define NVBLASTEXTAUTHORINGBONDGENERATOR_H

#include "NvBlastExtAuthoringTypes.h"

namespace physx
{
class PxPlane;
class PxCooking;
class PxPhysicsInsertionCallback;
}

struct NvBlastBondDesc;
struct NvBlastChunkDesc;
struct NvBlastBond;

namespace Nv
{
namespace Blast
{

// Forward declarations
class FractureTool;
class TriangleProcessor;
struct PlaneChunkIndexer;

/**
	Bond interface generation configuration
	EXACT - common surface will be searched
	AVERAGE - Inerface is approximated by projections or intersecitons with midplane
	maxSeparation - for AVERAGE mode. Maximum distance between chunks and midplane used in decision whether create bond or chunks are too far from each other.
*/
struct BondGenerationConfig
{
	enum BondGenMode { EXACT, AVERAGE };
	float maxSeparation;
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
	virtual ~BlastBondGenerator() {}

	/**
		Release BlastBondGenerator memory
	*/
	virtual void release() = 0;

	/**
		This method based on marking triangles during fracture process, so can be used only with internally fractured meshes.
		\note User should call NVBLAST_FREE for resultBondDescs when it not needed anymore
		\param[in]  tool					FractureTool which contains chunks representation, tool->finalizeFracturing() should be called before.
		\param[in]  chunkIsSupport			Pointer to array of flags, if true - chunk is support. Array size should be equal to chunk count in tool.
		\param[out] resultBondDescs			Pointer to array of created bond descriptors.
		\param[out] resultChunkDescriptors	Pointer to array of created chunk descriptors.
		\return								Number of created bonds
	*/
	virtual int32_t	buildDescFromInternalFracture(FractureTool* tool, const bool* chunkIsSupport, 
		NvBlastBondDesc*& resultBondDescs, NvBlastChunkDesc*& resultChunkDescriptors) = 0;


	/**
		Creates bond description between two meshes
		\param[in] meshACount		Number of triangles in mesh A
		\param[in] meshA			Pointer to array of triangles of mesh A.
		\param[in] meshBCount		Number of triangles in mesh B
		\param[in] meshB			Pointer to array of triangles of mesh B.
		\param[out] resultBond		Result bond description.
		\param[in] conf				Bond creation mode.
		\return						0 if success
	*/
	virtual int32_t	createBondBetweenMeshes(uint32_t meshACount, const Triangle* meshA, uint32_t meshBCount, const Triangle* meshB, 
		NvBlastBond& resultBond, BondGenerationConfig conf = BondGenerationConfig()) = 0;

	/**
		Creates bond description between number of meshes
		\note User should call NVBLAST_FREE for resultBondDescs when it not needed anymore
		\param[in] meshCount		Number of meshes
		\param[in] geometryOffset	Pointer to array of triangle offsets for each mesh. 
									Containts meshCount + 1 element, last one is total number of triangles in geometry
		\param[in] geometry			Pointer to array of triangles. 
									Triangles from geometryOffset[i] to geometryOffset[i+1] correspond to i-th mesh.
		\param[in] overlapsCount	Number of overlaps
		\param[in] overlaps			Pointer to array of pairs - indexes of chunks, for which bond should be created.
		\param[out] resultBond		Pointer to array of result bonds.
		\param[in] cfg				Bond creation mode.
		\return						Number of created bonds
	*/
	virtual int32_t	createBondBetweenMeshes(uint32_t meshCount, const uint32_t* geometryOffset, const Triangle* geometry,
		uint32_t overlapsCount, const uint32_t* overlapsA, const uint32_t* overlapsB, 
		NvBlastBondDesc*& resultBond, BondGenerationConfig cfg) = 0;


	/**
		Creates bond description for prefractured meshes, when there is no info about which chunks should be connected with bond.
		\note User should call NVBLAST_FREE for resultBondDescs when it not needed anymore
		\param[in] meshCount		Number of meshes
		\param[in] geometryOffset	Pointer to array of triangle offsets for each mesh. 
									Containts meshCount + 1 element, last one is total number of triangles in geometry
		\param[in] geometry			Pointer to array of triangles. 
									Triangles from geometryOffset[i] to geometryOffset[i+1] correspond to i-th mesh.
		\param[in] chunkIsSupport	Pointer to array of flags, if true - chunk is support. Array size should be equal to chunk count in tool.
		\param[out] resultBondDescs	Pointer to array of result bonds.
		\param[in] conf				Bond creation mode.
		\return						Number of created bonds
	*/
	virtual int32_t	bondsFromPrefractured(uint32_t meshCount, const uint32_t* geometryOffset, const Triangle* geometry,
		const bool* chunkIsSupport, NvBlastBondDesc*& resultBondDescs,
		BondGenerationConfig conf = BondGenerationConfig()) = 0;
	
	/**
		Creates bond description for prefractured meshes, when there is no info about which chunks should be connected with bond.
		This uses the same process as bondsFromPrefractured using the BondGenMode::AVERAGE mode however the existing collision data is used.
		\note User should call NVBLAST_FREE for resultBondDescs when it not needed anymore.
		\param[in] meshCount		Number of meshes
		\param[in] convexHullOffset	Pointer to array of convex hull offsets for each mesh. 
									Containts meshCount + 1 element, last one is total number of hulls in the geometry
		\param[in] chunkHulls		Pointer to array of convex hulls. 
									Hulls from convexHullOffset[i] to convexHullOffset[i+1] correspond to i-th mesh.
		\param[in] chunkIsSupport	Pointer to array of flags, if true - chunk is support. Array size should be equal to chunk count in tool.
		\param[in] meshGroups		Pointer to array of group ids for each mesh, bonds will not be generated between meshs of the same group. If null each mesh is assumed to be in it's own group.
		\param[out] resultBondDescs	Pointer to array of result bonds.
		\return						Number of created bonds
	*/
	virtual int32_t	bondsFromPrefractured(uint32_t meshCount, const uint32_t* convexHullOffset, const CollisionHull** chunkHulls,
		const bool* chunkIsSupport, const uint32_t* meshGroups, NvBlastBondDesc*& resultBondDescs, float maxSeparation) = 0;
};

}	// namespace Blast
}	// namespace Nv

#endif // NVBLASTEXTAUTHORINGBONDGENERATOR_H