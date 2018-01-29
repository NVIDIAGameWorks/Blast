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


#ifndef NVBLASTAUTHORINGFRACTURETOOL_H
#define NVBLASTAUTHORINGFRACTURETOOL_H

#include "NvBlastExtAuthoringTypes.h"

namespace Nv
{
namespace Blast
{

class SpatialAccelerator;
class Triangulator;
class Mesh;
class CutoutSet;

/*
	Chunk data, chunk with chunkId == 0 is always source mesh.
*/
struct ChunkInfo
{
	Mesh*	meshData;
	int32_t	parent;
	int32_t	chunkId;
	bool	isLeaf;
	bool	isChanged;
};

/*
	Noise fracturing configuration for chunks's faces
*/
struct NoiseConfiguration
{
	/**
	Noisy slicing configutaion:

	Amplitude of cutting surface noise. If it is 0 - noise is disabled.
	*/
	float	amplitude = 0.f;

	/**
	Frequencey of cutting surface noise.
	*/
	float	frequency = 1.f;

	/**
	Octave number in slicing surface noise.
	*/
	uint32_t octaveNumber = 1;

	/**
	Cutting surface resolution.
	*/
	uint32_t surfaceResolution = 1;
};

/*
	Slicing fracturing configuration
*/
struct SlicingConfiguration
{
	/**
		Number of slices in each direction
	*/
	int32_t	x_slices = 1, y_slices = 1, z_slices = 1;

	/**
		Offset variation, value in [0, 1]
	*/
	float	offset_variations = 0.f;

	/**
		Angle variation, value in [0, 1]
	*/
	float	angle_variations = 0.f;

	/*
		Noise parameters for faces between sliced chunks
	*/
	NoiseConfiguration noise;
};

/**
	Cutout fracturing configuration
*/
struct CutoutConfiguration
{
	/**
		Set of grouped convex loop patterns for cutout in normal direction.
		Not required for PLANE_ONLY mode
	*/
	CutoutSet* cutoutSet = nullptr;

	/**
		Transform for initial pattern position and orientation.
		By default 2d pattern lies in XY plane (Y is up) the center of pattern is (0, 0)
	*/
	physx::PxTransform transform = physx::PxTransform(physx::PxIdentity);

	/**
		Scale for pattern. Unscaled pattern has size (1, 1).
		For negative scale pattern will be placed at the center of chunk and scaled with max distance between points of its AABB
	*/
	physx::PxVec2 scale = physx::PxVec2(-1, -1);

	/**
		If relative transform is set - position will be displacement vector from chunk's center. Otherwise from global origin.
	*/
	bool isRelativeTransform = true;

	/**
		Noise parameters for cutout surface, see NoiseConfiguration.
	*/
	NoiseConfiguration noise;
};

/**
	Class for voronoi sites generation inside supplied mesh.
*/
class VoronoiSitesGenerator
{
public:
	virtual ~VoronoiSitesGenerator() {}

	/**
		Release VoronoiSitesGenerator memory
	*/
	virtual void						release() = 0;

	/**
		Set base fracture mesh
	*/
	virtual void						setBaseMesh(const Mesh* mesh) = 0;

	/**
		Access to generated voronoi sites.
		\param[out]				Pointer to generated voronoi sites
		\return					Count of generated voronoi sites.
	*/
	virtual uint32_t					getVoronoiSites(const physx::PxVec3*& sites) = 0;
	
	/**
		Add site in particular point
		\param[in] site		Site coordinates
	*/
	virtual void						addSite(const physx::PxVec3& site) = 0;
	/**
		Uniformly generate sites inside the mesh
		\param[in] numberOfSites	Number of generated sites
	*/
	virtual void						uniformlyGenerateSitesInMesh(uint32_t numberOfSites) = 0;

	/**
		Generate sites in clustered fashion
		\param[in] numberOfClusters	Number of generated clusters
		\param[in] sitesPerCluster	Number of sites in each cluster
		\param[in] clusterRadius	Voronoi cells cluster radius
	*/
	virtual void						clusteredSitesGeneration(uint32_t numberOfClusters, uint32_t sitesPerCluster, float clusterRadius) = 0;

	/**
		Radial pattern of sites generation
		\param[in] center		Center of generated pattern
		\param[in] normal		Normal to plane in which sites are generated
		\param[in] radius		Pattern radius
		\param[in] angularSteps	Number of angular steps
		\param[in] radialSteps	Number of radial steps
		\param[in] angleOffset	Angle offset at each radial step
		\param[in] variability	Randomness of sites distribution 
	*/
	virtual void						radialPattern(const physx::PxVec3& center, const physx::PxVec3& normal, float radius, int32_t angularSteps, int32_t radialSteps, float angleOffset = 0.0f, float variability = 0.0f) = 0;

	/**
		Generate sites inside sphere
		\param[in] count		Count of generated sites
		\param[in] radius		Radius of sphere
		\param[in] center		Center of sphere
	*/
	virtual void						generateInSphere(const uint32_t count, const float radius, const physx::PxVec3& center) = 0;

	/**
		Set stencil mesh. With stencil mesh sites are generated only inside both of fracture and stencil meshes. 
		\param[in] stencil		Stencil mesh.
	*/
	virtual void						setStencil(const Mesh* stencil) = 0;

	/**
		Removes stencil mesh
	*/
	virtual void						clearStencil() = 0;

	/** 
		Deletes sites inside supplied sphere
		\param[in] radius				Radius of sphere
		\param[in] center				Center of sphere
		\param[in] eraserProbability	Probability of removing some particular site
	*/
	virtual void						deleteInSphere(const float radius, const physx::PxVec3& center, const float eraserProbability = 1) = 0;
};

/**
	FractureTool class provides methods to fracture provided mesh and generate Blast asset data
*/
class FractureTool
{

public:
	virtual ~FractureTool() {}

	/**
		Release FractureTool memory
	*/
	virtual void									release() = 0;

	/**
		Reset FractureTool state.
	*/
	virtual void									reset() = 0;
	
	
	/**
		Set input mesh which will be fractured, FractureTool will be reseted.
	*/
	virtual void									setSourceMesh(const Mesh* mesh) = 0;

	/**
		Set chunk mesh, parentId should be valid, return id of new chunk.
	*/
	virtual int32_t									setChunkMesh(const Mesh* mesh, int32_t parentId) = 0;

	/**
	Set the material id to use for new interior faces. Defaults to MATERIAL_INTERIOR
	*/
	virtual void									setInteriorMaterialId(int32_t materialId) = 0;

	/**
	Gets the material id to use for new interior faces
	*/
	virtual int32_t									getInteriorMaterialId() const = 0;

	/**
	Replaces an material id on faces with a new one
	*/
	virtual void									replaceMaterialId(int32_t oldMaterialId, int32_t newMaterialId) = 0;

	/**
		Get chunk mesh in polygonal representation. User's code should release it after usage.
	*/
	virtual Mesh*									createChunkMesh(int32_t chunkId) = 0;

	/**
		Input mesh is scaled and transformed internally to fit unit cube centered in origin.
		Method provides offset vector and scale parameter;
	*/
	virtual void									getTransformation(physx::PxVec3& offset, float& scale) = 0;


	/**
		Fractures specified chunk with voronoi method.
		\param[in] chunkId				Chunk to fracture
		\param[in] cellPoints			Array of voronoi sites
		\param[in] replaceChunk			if 'true', newly generated chunks will replace source chunk, if 'false', newly generated chunks will be at next depth level, source chunk will be parent for them.
										Case replaceChunk == true && chunkId == 0 considered as wrong input parameters
		\return   If 0, fracturing is successful.
	*/
	virtual int32_t									voronoiFracturing(uint32_t chunkId, uint32_t cellCount, const physx::PxVec3* cellPoints, bool replaceChunk) = 0;

	/**
		Fractures specified chunk with voronoi method. Cells can be scaled along x,y,z axes.
		\param[in] chunkId				Chunk to fracture
		\param[in] cellPoints			Array of voronoi sites
		\param[in] cellPoints			Array of voronoi sites
		\param[in] scale				Voronoi cells scaling factor
		\param[in] rotation				Voronoi cells rotation. Has no effect without cells scale factor
		\param[in] replaceChunk			if 'true', newly generated chunks will replace source chunk, if 'false', newly generated chunks will be at next depth level, source chunk will be parent for them.
									    Case replaceChunk == true && chunkId == 0 considered as wrong input parameters
		\return   If 0, fracturing is successful.
	*/
	virtual int32_t									voronoiFracturing(uint32_t chunkId, uint32_t cellCount, const physx::PxVec3* cellPoints, const physx::PxVec3& scale, const physx::PxQuat& rotation, bool replaceChunk) = 0;


	/**
		Fractures specified chunk with slicing method.
		\param[in] chunkId				Chunk to fracture
		\param[in] conf					Slicing parameters, see SlicingConfiguration.
		\param[in] replaceChunk			if 'true', newly generated chunks will replace source chunk, if 'false', newly generated chunks will be at next depth level, source chunk will be parent for them.
										Case replaceChunk == true && chunkId == 0 considered as wrong input parameters
		\param[in] rnd					User supplied random number generator

		\return   If 0, fracturing is successful.
	*/
	virtual int32_t									slicing(uint32_t chunkId, const SlicingConfiguration& conf, bool replaceChunk, RandomGeneratorBase* rnd) = 0;

	/**
		Cut chunk with plane.
		\param[in] chunkId				Chunk to fracture
		\param[in] normal				Plane normal
		\param[in] position				Point on plane
		\param[in] noise				Noise configuration for plane-chunk intersection, see NoiseConfiguration.
		\param[in] replaceChunk			if 'true', newly generated chunks will replace source chunk, if 'false', newly generated chunks will be at next depth level, source chunk will be parent for them.
		Case replaceChunk == true && chunkId == 0 considered as wrong input parameters
		\param[in] rnd					User supplied random number generator

		\return   If 0, fracturing is successful.
	*/
	virtual int32_t									cut(uint32_t chunkId, const physx::PxVec3& normal, const physx::PxVec3& position, const NoiseConfiguration& noise, bool replaceChunk, RandomGeneratorBase* rnd) = 0;

	/**
		Cutout fracture for specified chunk.
		\param[in] chunkId				Chunk to fracture
		\param[in] conf					Cutout parameters, see CutoutConfiguration.
		\param[in] replaceChunk			if 'true', newly generated chunks will replace source chunk, if 'false', newly generated chunks will be at next depth level, source chunk will be parent for them.
										Case replaceChunk == true && chunkId == 0 considered as wrong input parameters
		\param[in] rnd					User supplied random number generator

		\return   If 0, fracturing is successful.
	*/
	virtual int32_t									cutout(uint32_t chunkId, CutoutConfiguration conf, bool replaceChunk, RandomGeneratorBase* rnd) = 0;


	/**
		Creates resulting fractured mesh geometry from intermediate format
	*/
	virtual void									finalizeFracturing() = 0;
	
	/**
		Returns overall number of chunks in fracture.
	*/
	virtual uint32_t								getChunkCount() const = 0;

	/**
		Get chunk information
	*/
	virtual const ChunkInfo&    					getChunkInfo(int32_t chunkIndex) = 0;

	/**
		Get percentage of mesh overlap.
		percentage computed as volume(intersection(meshA , meshB)) / volume (meshA) 
		\param[in] meshA Mesh A
		\param[in] meshB Mesh B
		\return mesh overlap percentage
	*/
	virtual float									getMeshOverlap(const Mesh& meshA, const Mesh& meshB) = 0;

	/**
		Get chunk base mesh
		\param[in] chunkIndex Chunk index
		\param[out] output Array of triangles to be filled
		\return number of triangles in base mesh
	*/
	virtual uint32_t								getBaseMesh(int32_t chunkIndex, Triangle*& output) = 0;

	/**
		Update chunk base mesh
		\note Doesn't allocates output array, Triangle* output should be preallocated by user
		\param[in] chunkIndex Chunk index
		\param[out] output Array of triangles to be filled
		\return number of triangles in base mesh
	*/
	virtual uint32_t								updateBaseMesh(int32_t chunkIndex, Triangle* output) = 0;

	/**
		Return index of chunk with specified chunkId
		\param[in] chunkId Chunk ID
		\return Chunk index in internal buffer, if not exist -1 is returned.
	*/
	virtual int32_t									getChunkIndex(int32_t chunkId) = 0;

	/**
		Return id of chunk with specified index.
		\param[in] chunkIndex Chunk index
		\return Chunk id or -1 if there is no such chunk.
	*/
	virtual int32_t									getChunkId(int32_t chunkIndex) = 0;

	/**
		Return depth level of the given chunk
		\param[in] chunkId Chunk ID
		\return Chunk depth or -1 if there is no such chunk.
	*/
	virtual int32_t									getChunkDepth(int32_t chunkId) = 0;

	/**
		Return array of chunks IDs with given depth.
		\param[in]  depth Chunk depth
		\param[out] Pointer to array of chunk IDs
		\return Number of chunks in array
	*/
	virtual uint32_t								getChunksIdAtDepth(uint32_t depth, int32_t*& chunkIds) = 0;

	/**
		Get result geometry without noise as vertex and index buffers, where index buffers contain series of triplets
		which represent triangles.
		\param[out] vertexBuffer Array of vertices to be filled
		\param[out] indexBuffer Array of indices to be filled
		\param[out] indexBufferOffsets Array of offsets in indexBuffer for each base mesh. 
					Contains getChunkCount() + 1 elements. Last one is indexBuffer size
		\return Number of vertices in vertexBuffer
	*/
	virtual uint32_t								getBufferedBaseMeshes(Vertex*& vertexBuffer, uint32_t*& indexBuffer, uint32_t*& indexBufferOffsets) = 0;

	/**
		Set automatic islands removing. May cause instabilities.
		\param[in] isRemoveIslands Flag whether remove or not islands.
	*/
	virtual void									setRemoveIslands(bool isRemoveIslands) = 0;

	/**
		Try find islands and remove them on some specifical chunk. If chunk has childs, island removing can lead to wrong results! Apply it before further chunk splitting.
		\param[in] chunkId Chunk ID which should be checked for islands
		\return Number of found islands is returned
	*/
	virtual int32_t									islandDetectionAndRemoving(int32_t chunkId) = 0;

	/**
		Check if input mesh contains open edges. Open edges can lead to wrong fracturing results.
		\return true if mesh contains open edges
	*/
	virtual bool									isMeshContainOpenEdges(const Mesh* input) = 0;

	/**
		Delete all children for specified chunk (also recursively delete chidren of children).
		\param[in] chunkId Chunk ID which children should be deleted
		\return true if one or more chunks were removed
	*/
	virtual bool									deleteAllChildrenOfChunk(int32_t chunkId) = 0;

	/**
		Optimize chunk hierarhy for better runtime performance. 
		It tries to unite chunks to groups of some size in order to transform flat hierarchy (all chunks are children of single root) 
		to tree like hieracrhy with limited number of children for each chunk.
		\param[in] maxAtLevel	If number of children of some chunk less then maxAtLevel then it would be considered as already optimized and skipped.
		\param[in] maxGroupSize	Max number of children for processed chunks.
	*/
	virtual void									uniteChunks(uint32_t maxAtLevel, uint32_t maxGroupSize) = 0;

	/**
		Rescale interior uv coordinates of given chunk to fit square of given size.
		\param[in] side Size of square side
		\param[in] chunkId Chunk ID for which UVs should be scaled.
	*/
	virtual void									fitUvToRect(float side, uint32_t chunkId) = 0;

	/**
		Rescale interior uv coordinates of all existing chunks to fit square of given size, relative sizes will be preserved.
		\param[in] side Size of square side
	*/
	virtual void									fitAllUvToRect(float side) = 0;

};

} // namespace Blast
} // namespace Nv

#endif // ifndef NVBLASTAUTHORINGFRACTURETOOL_H
