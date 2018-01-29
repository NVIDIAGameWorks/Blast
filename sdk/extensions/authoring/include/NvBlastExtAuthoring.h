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


#ifndef NVBLASTAUTHORING_H
#define NVBLASTAUTHORING_H

#include "NvBlastExtAuthoringTypes.h"

namespace physx
{
	class PxCooking;
	class PxPhysicsInsertionCallback;
}

namespace Nv
{
	namespace Blast
	{
		class Mesh;
		class VoronoiSitesGenerator;
		class CutoutSet;
		class FractureTool;
		class ConvexMeshBuilder;
		class BlastBondGenerator;
		class MeshCleaner;
		struct CollisionParams;
		struct CollisionHull;
	}
}

struct NvBlastExtAssetUtilsBondDesc;

/**
Constructs mesh object from array of triangles.
User should call release() after usage.

\param[in] positions		Array for vertex positions, 3 * verticesCount floats will be read
\param[in] normals			Array for vertex normals, 3 * verticesCount floats will be read
\param[in] uv				Array for vertex uv coordinates, 2 * verticesCount floats will be read
\param[in] verticesCount	Number of vertices in mesh
\param[in] indices			Array of vertex indices. Indices contain vertex index triplets which form a mesh triangle.
\param[in] indicesCount		Indices count (should be equal to numberOfTriangles * 3)

\return pointer to Nv::Blast::Mesh if it was created succefully otherwise return nullptr
*/
NVBLAST_API Nv::Blast::Mesh* NvBlastExtAuthoringCreateMesh(const physx::PxVec3* positions, const physx::PxVec3* normals,
	const physx::PxVec2* uv, uint32_t verticesCount, const uint32_t* indices, uint32_t indicesCount);

/**
Constructs mesh object from array of vertices, edges and facets.
User should call release() after usage.

\param[in] vertices			Array for Nv::Blast::Vertex
\param[in] edges			Array for Nv::Blast::Edge
\param[in] facets			Array for Nv::Blast::Facet
\param[in] verticesCount	Number of vertices in mesh
\param[in] edgesCount		Number of edges in mesh
\param[in] facetsCount		Number of facets in mesh

\return pointer to Nv::Blast::Mesh if it was created succefully otherwise return nullptr
*/
NVBLAST_API Nv::Blast::Mesh* NvBlastExtAuthoringCreateMeshFromFacets(const void* vertices, const void* edges, const void* facets,
	uint32_t verticesCount, uint32_t edgesCount, uint32_t facetsCount);

/**
Voronoi sites should not be generated outside of the fractured mesh, so VoronoiSitesGenerator
should be supplied with fracture mesh.
\param[in] mesh			Fracture mesh
\param[in] rnd			User supplied random value generator.
\return					Pointer to VoronoiSitesGenerator. User's code should release it after usage.
*/
NVBLAST_API Nv::Blast::VoronoiSitesGenerator* NvBlastExtAuthoringCreateVoronoiSitesGenerator(Nv::Blast::Mesh* mesh,
	Nv::Blast::RandomGeneratorBase* rng);

/** Instantiates a blank CutoutSet */
NVBLAST_API Nv::Blast::CutoutSet* NvBlastExtAuthoringCreateCutoutSet();

/**
Builds a cutout set (which must have been initially created by createCutoutSet()).
Uses a bitmap described by pixelBuffer, bufferWidth, and bufferHeight.  Each pixel is represented
by one byte in the buffer.

\param cutoutSet		the CutoutSet to build
\param pixelBuffer		pointer to be beginning of the pixel buffer
\param bufferWidth		the width of the buffer in pixels
\param bufferHeight		the height of the buffer in pixels
\param segmentationErrorThreshold	Reduce the number of vertices on curve untill segmentation error is smaller then specified. By default set it to 0.001.
\param snapThreshold	the pixel distance at which neighboring cutout vertices and segments may be fudged into alignment. By default set it to 1.
\param periodic			whether or not to use periodic boundary conditions when creating cutouts from the map
\param expandGaps		expand cutout regions to gaps or keep it as is

*/
NVBLAST_API void NvBlastExtAuthoringBuildCutoutSet(Nv::Blast::CutoutSet& cutoutSet, const uint8_t* pixelBuffer, 
	uint32_t bufferWidth, uint32_t bufferHeight, float segmentationErrorThreshold, float snapThreshold, bool periodic, bool expandGaps);

/**
Create FractureTool object.
\return Pointer to create FractureTool. User's code should release it after usage.
*/
NVBLAST_API Nv::Blast::FractureTool* NvBlastExtAuthoringCreateFractureTool();

/**
Create BlastBondGenerator
\return Pointer to created BlastBondGenerator. User's code should release it after usage.
*/
NVBLAST_API Nv::Blast::BlastBondGenerator* NvBlastExtAuthoringCreateBondGenerator(physx::PxCooking* cooking, 
	physx::PxPhysicsInsertionCallback* insertionCallback);

/**
Create ConvexMeshBuilder
\return Pointer to created ConvexMeshBuilder. User's code should release it after usage.
*/
NVBLAST_API Nv::Blast::ConvexMeshBuilder* NvBlastExtAuthoringCreateConvexMeshBuilder(physx::PxCooking* cooking,
	physx::PxPhysicsInsertionCallback* insertionCallback);


/**
Transforms collision hull in place using scale, rotation, transform.
\param[in, out]	hull		Pointer to the hull to be transformed (modified).
\param[in]		scale		Pointer to scale to be applied. Can be nullptr.
\param[in]		rotation	Pointer to rotation to be applied. Can be nullptr.
\param[in]		translation	Pointer to translation to be applied. Can be nullptr.
*/
NVBLAST_API void NvBlastExtAuthoringTransformCollisionHullInPlace
(
	Nv::Blast::CollisionHull* hull,
	const physx::PxVec3* scaling,
	const physx::PxQuat* rotation,
	const physx::PxVec3* translation
);

/**
Transforms collision hull in place using scale, rotation, transform.
\param[in]	hull		Pointer to the hull to be transformed (modified).
\param[in]		scale		Pointer to scale to be applied. Can be nullptr.
\param[in]		rotation	Pointer to rotation to be applied. Can be nullptr.
\param[in]		translation	Pointer to translation to be applied. Can be nullptr.
*/
NVBLAST_API Nv::Blast::CollisionHull* NvBlastExtAuthoringTransformCollisionHull
(
	const Nv::Blast::CollisionHull* hull,
	const physx::PxVec3* scaling,
	const physx::PxQuat* rotation,
	const physx::PxVec3* translation
);

/**
Performs pending fractures and generates fractured asset, render and collision geometry

\param[in]  fTool				Fracture tool created by NvBlastExtAuthoringCreateFractureTool
\param[in]  bondGenerator		Bond generator created by NvBlastExtAuthoringCreateBondGenerator
\param[in]  collisionBuilder	Collision builder created by NvBlastExtAuthoringCreateConvexMeshBuilder
\param[in]  defaultSupportDepth All new chunks will be marked as support if its depth equal to defaultSupportDepth. 
								By default leaves (chunks without children) marked as support.
\param[in]  collisionParam		Parameters of collision hulls generation. 
\return		Authoring result
*/
NVBLAST_API Nv::Blast::AuthoringResult* NvBlastExtAuthoringProcessFracture(Nv::Blast::FractureTool& fTool,
	Nv::Blast::BlastBondGenerator& bondGenerator, Nv::Blast::ConvexMeshBuilder& collisionBuilder, const Nv::Blast::CollisionParams& collisionParam, int32_t defaultSupportDepth = -1);

/**
Updates graphics mesh only

\param[in]  fTool				Fracture tool created by NvBlastExtAuthoringCreateFractureTool
\param[out] ares				AuthoringResult object which contains chunks, for which rendermeshes will be updated (e.g. to tweak UVs). Initially should be created by NvBlastExtAuthoringProcessFracture.
*/
NVBLAST_API void NvBlastExtAuthoringUpdateGraphicsMesh(Nv::Blast::FractureTool& fTool, Nv::Blast::AuthoringResult& ares);

/**
Build collision meshes

\param[in,out]	ares				AuthoringResult object which contains chunks, for which collision meshes will be built.
\param[in]		collisionBuilder	Reference to ConvexMeshBuilder instance.
\param[in]		collisionParam		Parameters of collision hulls generation.
\param[in]		chunksToProcessCount Number of chunk indices in chunksToProcess memory buffer.
\param[in]		chunksToProcess		Chunk indices for which collision mesh should be built.
*/
NVBLAST_API void NvBlastExtAuthoringBuildCollisionMeshes
(
	Nv::Blast::AuthoringResult& ares, 
	Nv::Blast::ConvexMeshBuilder& collisionBuilder, 
	const Nv::Blast::CollisionParams& collisionParam, 
	uint32_t chunksToProcessCount,
	uint32_t* chunksToProcess
);

/**
	Creates MeshCleaner object
	\return pointer to Nv::Blast::Mesh if it was created succefully otherwise return nullptr
*/
NVBLAST_API Nv::Blast::MeshCleaner* NvBlastExtAuthoringCreateMeshCleaner();

/**
Finds bonds connecting chunks in a list of assets

New bond descriptors may be given to bond support chunks from different components.

An NvBlastAsset may appear more than once in the components array.

NOTE: This function allocates memory using the allocator in NvBlastGlobals, to create the new bond
descriptor arrays returned. The user must free this memory after use with NVBLAST_FREE

\param[in]	components			An array of assets to merge, of size componentCount.
\param[in]	scales				If not NULL, an array of size componentCount of scales to apply to the geometric data in the chunks and bonds. If NULL, no scaling is applied.
\param[in]	rotations			If not NULL, an array of size componentCount of rotations to apply to the geometric data in the chunks and bonds.  The quaternions MUST be normalized.
								If NULL, no rotations are applied.
\param[in]	translations		If not NULL, an array of of size componentCount of translations to apply to the geometric data in the chunks and bonds.  If NULL, no translations are applied.
\param[in]	convexHullOffsets	For each component, an array of chunkSize+1 specifying the start of the convex hulls for that chunk inside the chunkHulls array for that component.
\param[in]	chunkHulls			For each component, an array of CollisionHull* specifying the collision geometry for the chunks in that component.
\param[in]	componentCount		The size of the components and relativeTransforms arrays.
\param[out]	newBondDescs		Descriptors of type NvBlastExtAssetUtilsBondDesc for new bonds between components.
\param[in]	maxSeparation		Maximal distance between chunks which can be connected by bond.
\return the number of bonds in newBondDescs
*/
NVBLAST_API uint32_t NvBlastExtAuthoringFindAssetConnectingBonds
(
	const NvBlastAsset** components,
	const physx::PxVec3* scales,
	const physx::PxQuat* rotations,
	const physx::PxVec3* translations,
	const uint32_t** convexHullOffsets,
	const Nv::Blast::CollisionHull*** chunkHulls,
	uint32_t componentCount,
	NvBlastExtAssetUtilsBondDesc*& newBondDescs,
	float maxSeparation = 0.0f
);

#endif // ifndef NVBLASTAUTHORING_H
