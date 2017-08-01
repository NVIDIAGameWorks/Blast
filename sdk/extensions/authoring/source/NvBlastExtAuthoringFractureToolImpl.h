/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTAUTHORINGFRACTURETOOLIMPL_H
#define NVBLASTAUTHORINGFRACTURETOOLIMPL_H

#include "NvBlastExtAuthoringFractureTool.h"
#include "NvBlastExtAuthoringMesh.h"
#include <vector>

namespace Nv
{
namespace Blast
{

class SpatialAccelerator;
class Triangulator;


/**
	Class for voronoi sites generation inside supplied mesh.
*/
class VoronoiSitesGeneratorImpl : public VoronoiSitesGenerator
{
public:
	
	/** 
		Voronoi sites should not be generated outside of the fractured mesh, so VoronoiSitesGenerator
		should be supplied with fracture mesh.
		\param[in] mesh			Fracture mesh
		\param[in] rnd			User supplied random value generator.
		\return
	*/
	VoronoiSitesGeneratorImpl(const Mesh* mesh, RandomGeneratorBase* rnd);
	~VoronoiSitesGeneratorImpl();

	void						release() override;

	/**
		Set base fracture mesh
	*/
	void						setBaseMesh(const Mesh* m) override;

	/**
		Access to generated voronoi sites.
		\note User should call NVBLAST_FREE for hulls and hullsOffset when it not needed anymore
		\param[out]				Pointer to generated voronoi sites
		\return					Count of generated voronoi sites.
	*/
	 uint32_t					getVoronoiSites(const physx::PxVec3*& sites) override;
	
	/**
		Add site in particular point
		\param[in] site		Site coordinates
	*/
	void						addSite(const physx::PxVec3& site) override;
	/**
		Uniformly generate sites inside the mesh
		\param[in] numberOfSites	Number of generated sites
	*/
	void						uniformlyGenerateSitesInMesh(uint32_t numberOfSites) override;

	/**
		Generate sites in clustered fashion
		\param[in] numberOfClusters	Number of generated clusters
		\param[in] sitesPerCluster	Number of sites in each cluster
		\param[in] clusterRadius	Voronoi cells cluster radius
	*/
	void						clusteredSitesGeneration(uint32_t numberOfClusters, uint32_t sitesPerCluster, float clusterRadius) override;

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
	void						radialPattern(const physx::PxVec3& center, const physx::PxVec3& normal, float radius, int32_t angularSteps, int32_t radialSteps, float angleOffset = 0.0f, float variability = 0.0f) override;

	/**
		Generate sites inside sphere
		\param[in] count		Count of generated sites
		\param[in] radius		Radius of sphere
		\param[in] center		Center of sphere
	*/
	void						generateInSphere(const uint32_t count, const float radius, const physx::PxVec3& center) override;
	/**
		Set stencil mesh. With stencil mesh sites are generated only inside both of fracture and stencil meshes. 
		\param[in] stencil		Stencil mesh.
	*/
	void						setStencil(const Mesh* stencil) override;
	/**
		Removes stencil mesh
	*/
	void						clearStencil() override;

	/** 
		Deletes sites inside supplied sphere
		\param[in] radius				Radius of sphere
		\param[in] center				Center of sphere
		\param[in] eraserProbability	Probability of removing some particular site
	*/
	void						deleteInSphere(const float radius, const physx::PxVec3& center, const float eraserProbability = 1) override;

private:
	std::vector <physx::PxVec3>	mGeneratedSites;
	const Mesh*						mMesh;
	const Mesh*						mStencil;
	RandomGeneratorBase*		mRnd;
	SpatialAccelerator*			mAccelerator;
};



/**
	FractureTool class provides methods to fracture provided mesh and generate Blast asset data
*/
class FractureToolImpl : public FractureTool
{

public:

	/**
		FractureTool can log asset creation info if logCallback is provided.
	*/
	FractureToolImpl()
	{
		mPlaneIndexerOffset = 1;
		mChunkIdCounter = 0;
		mRemoveIslands = false;
	}

	~FractureToolImpl()
	{
		reset();
	}

	void									release() override;

	/**
		Reset FractureTool state.
	*/
	void									reset() override;
	
	
	/**
		Set input mesh wich will be fractured, FractureTool will be reseted.
	*/
	void									setSourceMesh(const Mesh* mesh) override;

	/**
		Get chunk mesh in polygonal representation
	*/
	Mesh*									createChunkMesh(int32_t chunkId) override;

	/**
		Input mesh is scaled and transformed internally to fit unit cube centered in origin.
		Method provides offset vector and scale parameter;
	*/
	void									getTransformation(physx::PxVec3& offset, float& scale) override;


	/**
		Fractures specified chunk with voronoi method.
		\param[in] chunkId				Chunk to fracture
		\param[in] cellPoints			Array of voronoi sites
		\param[in] replaceChunk			if 'true', newly generated chunks will replace source chunk, if 'false', newly generated chunks will be at next depth level, source chunk will be parent for them.
										Case replaceChunk == true && chunkId == 0 considered as wrong input parameters
		\return   If 0, fracturing is successful.
	*/
	int32_t									voronoiFracturing(uint32_t chunkId, uint32_t cellCount, const physx::PxVec3* cellPoints, bool replaceChunk) override;

	/**
		Fractures specified chunk with voronoi method. Cells can be scaled along x,y,z axes.
		\param[in] chunkId				Chunk to fracture
		\param[in] cellPoints			Array of voronoi sites
		\param[in] cellPoints			Array of voronoi sites
		\param[in] scale				Voronoi cells scaling factor
		\param[in] replaceChunk			if 'true', newly generated chunks will replace source chunk, if 'false', newly generated chunks will be at next depth level, source chunk will be parent for them.
									    Case replaceChunk == true && chunkId == 0 considered as wrong input parameters
		\return   If 0, fracturing is successful.
	*/
	int32_t									voronoiFracturing(uint32_t chunkId, uint32_t cellCount, const physx::PxVec3* cellPoints, const physx::PxVec3& scale, bool replaceChunk) override;


	/**
		Fractures specified chunk with slicing method.
		\param[in] chunkId				Chunk to fracture
		\param[in] conf					Slicing parameters, see SlicingConfiguration.
		\param[in] replaceChunk			if 'true', newly generated chunks will replace source chunk, if 'false', newly generated chunks will be at next depth level, source chunk will be parent for them.
										Case replaceChunk == true && chunkId == 0 considered as wrong input parameters
		\param[in] rnd					User supplied random number generator

		\return   If 0, fracturing is successful.
	*/
	int32_t									slicing(uint32_t chunkId, SlicingConfiguration conf, bool replaceChunk, RandomGeneratorBase* rnd) override;


	/**
		Creates resulting fractured mesh geometry from intermediate format
	*/
	void									finalizeFracturing() override;
	
	uint32_t								getChunkCount() const override;

	/**
		Get chunk information
	*/
	const ChunkInfo&    					getChunkInfo(int32_t chunkIndex) override;

	/**
		Get percentage of mesh overlap.
		percentage computed as volume(intersection(meshA , meshB)) / volume (meshA) 
		\param[in] meshA Mesh A
		\param[in] meshB Mesh B
		\return mesh overlap percentage
	*/
	float									getMeshOverlap(const Mesh& meshA, const Mesh& meshB) override;

	/**
		Get chunk base mesh
		\note User should call NVBLAST_FREE for output when it not needed anymore
		\param[in] chunkIndex Chunk index
		\param[out] output Array of triangles to be filled
		\return number of triangles in base mesh
	*/
	uint32_t								getBaseMesh(int32_t chunkIndex, Triangle*& output) override;

	/**
		Return index of chunk with specified chunkId
		\param[in] chunkId Chunk ID
		\return Chunk index in internal buffer, if not exist -1 is returned.
	*/
	int32_t									getChunkIndex(int32_t chunkId) override;

	/**
		Return id of chunk with specified index.
		\param[in] chunkIndex Chunk index
		\return Chunk id or -1 if there is no such chunk.
	*/
	int32_t									getChunkId(int32_t chunkIndex) override;

	/**
		Return depth level of the given chunk
		\param[in] chunkId Chunk ID
		\return Chunk depth or -1 if there is no such chunk.
	*/
	int32_t									getChunkDepth(int32_t chunkId) override;

	/**
		Return array of chunks IDs with given depth.
		\note User should call NVBLAST_FREE for chunkIds when it not needed anymore
		\param[in]  depth Chunk depth
		\param[out] Pointer to array of chunk IDs
		\return Number of chunks in array
	*/
	uint32_t								getChunksIdAtDepth(uint32_t depth, int32_t*& chunkIds) override;


	/**
		Get result geometry without noise as vertex and index buffers, where index buffers contain series of triplets
		which represent triangles.
		\note User should call NVBLAST_FREE for vertexBuffer, indexBuffer and indexBufferOffsets when it not needed anymore
		\param[out] vertexBuffer Array of vertices to be filled
		\param[out] indexBuffer Array of indices to be filled
		\param[out] indexBufferOffsets Array of offsets in indexBuffer for each base mesh. 
					Contains getChunkCount() + 1 elements. Last one is indexBuffer size
		\return Number of vertices in vertexBuffer
	*/
	uint32_t								getBufferedBaseMeshes(Vertex*& vertexBuffer, uint32_t*& indexBuffer, uint32_t*& indexBufferOffsets) override;

	/**
		Set automatic islands removing. May cause instabilities.
		\param[in] isRemoveIslands Flag whether remove or not islands.
	*/
	void									setRemoveIslands(bool isRemoveIslands) override;

	/**
		Try find islands and remove them on some specifical chunk. If chunk has childs, island removing can lead to wrong results! Apply it before further chunk splitting.
		\param[in] chunkId Chunk ID which should be checked for islands
		\return Number of found islands is returned
	*/
	int32_t									islandDetectionAndRemoving(int32_t chunkId) override;

	/**
		Check if input mesh contains open edges. Open edges can lead to wrong fracturing results.
		\return true if mesh contains open edges
	*/
	bool									isMeshContainOpenEdges(const Mesh* input) override;

private:	
	void									eraseChunk(int32_t chunkId);	
	bool									isAncestorForChunk(int32_t ancestorId, int32_t chunkId);
	void									deleteAllChildsOfChunk(int32_t chunkId);
	int32_t									slicingNoisy(uint32_t chunkId, SlicingConfiguration conf, bool replaceChunk, RandomGeneratorBase* rnd);

protected:
	/**
	Mesh scaled to unite-cube and translated to the origin
	*/
	float								mScaleFactor;
	physx::PxVec3						mOffset;

	/* Chunk mesh wrappers */
	std::vector<Triangulator*>	        mChunkPostprocessors;


	
	int32_t								mPlaneIndexerOffset;
	int32_t								mChunkIdCounter;
	std::vector<ChunkInfo>				mChunkData;

	bool								mRemoveIslands;
};

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTAUTHORINGFRACTURETOOLIMPL_H
