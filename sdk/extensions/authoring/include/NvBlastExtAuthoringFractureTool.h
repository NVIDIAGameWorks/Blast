/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTAUTHORINGFRACTURETOOL_H
#define NVBLASTAUTHORINGFRACTURETOOL_H

#include "NvBlastExtAuthoringMesh.h"
#include "NvBlastTypes.h"


namespace Nv
{
namespace Blast
{

class SpatialAccelerator;
class ChunkPostProcessor;


/*
	Chunk data, chunk with chunkId == 0 is always source mesh.
*/
struct ChunkInfo
{
	Mesh*	meshData;
	int32_t	parent;
	int32_t	chunkId;
	bool	isLeaf;
};


/*
	Slicing fracturing configuration


	default:
	x_slices = 1;
	y_slices = 1;
	z_slices = 1;

	offset_variations	= 0.f;
	angle_variations	= 0.f;
	noiseAmplitude		= 0.f;
	noiseFrequency		= 1.f;
	noiseOctaveNumber	= 1;
	surfaceResolution	= 1;
*/
struct SlicingConfiguration
{
	/** 
		Number of slices in each direction
	*/
	int32_t	x_slices, y_slices, z_slices;
	
	/** 
		Offset variation, value in [0, 1]
	*/
	float	offset_variations;
	/** 
		Angle variation, value in [0, 1]
	*/
	float	angle_variations;


	/**
		Noisy slicing configutaion:

		Amplitude of cutting surface noise. If it is 0 - noise is disabled.
	*/
	float noiseAmplitude;
	/**
		Frequencey of cutting surface noise. 
	*/
	float noiseFrequency;
	/**
		Octave number in slicing surface noise.
	*/
	uint32_t noiseOctaveNumber;
	/**
		Cutting surface resolution.
	*/
	int32_t surfaceResolution;


	SlicingConfiguration()
	{
		reset();
	}
	/**
		Set default params.
	*/
	void reset()
	{
		x_slices = 1;
		y_slices = 1;
		z_slices = 1;

		offset_variations	= 0.f;
		angle_variations	= 0.f;
		noiseAmplitude		= 0.f;
		noiseFrequency		= 1.f;
		noiseOctaveNumber	= 1;
		surfaceResolution	= 1;
	}

};



/**
	Class for voronoi sites generation inside supplied mesh.
*/
class VoronoiSitesGenerator
{
public:
	
	/** 
		Voronoi sites should not be generated outside of the fractured mesh, so VoronoiSitesGenerator
		should be supplied with fracture mesh.
		\param[in] mesh			Fracture mesh
		\param[in] rnd			User supplied random value generator.
		\return
	*/
	VoronoiSitesGenerator(Mesh* mesh, RandomGeneratorBase* rnd);
	~VoronoiSitesGenerator();

	/**
		Set base fracture mesh
	*/
	void						setBaseMesh(Mesh* m);

	/**
		Returns reference on vector of generated voronoi sites.
	*/
	std::vector<physx::PxVec3>& getVoronoiSites();
	
	/**
		Add site in particular point
		\param[in] site		Site coordinates
	*/
	void						addSite(const physx::PxVec3& site);
	/**
		Uniformly generate sites inside the mesh
		\param[in] numberOfSites	Number of generated sites
	*/
	void						uniformlyGenerateSitesInMesh(const uint32_t numberOfSites);

	/**
		Generate sites in clustered fashion
		\param[in] numberOfClusters	Number of generated clusters
		\param[in] sitesPerCluster	Number of sites in each cluster
		\param[in] clusterRadius	Voronoi cells cluster radius
	*/
	void						clusteredSitesGeneration(const uint32_t numberOfClusters, const uint32_t sitesPerCluster, float clusterRadius);

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
	void						radialPattern(const physx::PxVec3& center, const physx::PxVec3& normal, float radius, int32_t angularSteps, int32_t radialSteps, float angleOffset = 0.0f, float variability = 0.0f);

	/**
		Generate sites inside sphere
		\param[in] count		Count of generated sites
		\param[in] radius		Radius of sphere
		\param[in] center		Center of sphere
	*/
	void						generateInSphere(const uint32_t count, const float radius, const physx::PxVec3& center);
	/**
		Set stencil mesh. With stencil mesh sites are generated only inside both of fracture and stencil meshes. 
		\param[in] stencil		Stencil mesh.
	*/
	void						setStencil(Mesh* stencil);
	/**
		Removes stencil mesh
	*/
	void						clearStencil();

	/** 
		Deletes sites inside supplied sphere
		\param[in] radius				Radius of sphere
		\param[in] center				Center of sphere
		\param[in] eraserProbability	Probability of removing some particular site
	*/
	void						deleteInSphere(const float radius, const physx::PxVec3& center, const float eraserProbability = 1);

private:
	std::vector<physx::PxVec3>	mGeneratedSites;
	Mesh*						mMesh;
	Mesh*						mStencil;
	RandomGeneratorBase*		mRnd;
	SpatialAccelerator*			mAccelerator;
};



/**
	FractureTool class provides methods to fracture provided mesh and generate Blast asset data
*/
class FractureTool
{

public:

	/**
		FractureTool can log asset creation info if logCallback is provided.
	*/
	FractureTool(NvBlastLog logCallback = nullptr)
	{
		mPlaneIndexerOffset = 1;
		mChunkIdCounter = 0;
		mRemoveIslands = false;
		mLoggingCallback = logCallback;
	}

	~FractureTool()
	{
		reset();
	}

	/**
		Reset FractureTool state.
	*/
	void									reset();
	
	
	/**
		Set input mesh wich will be fractured, FractureTool will be reseted.
	*/
	void									setSourceMesh(Mesh* mesh);

	/**
		Get chunk mesh in polygonal representation
	*/
	Mesh									getChunkMesh(int32_t chunkId);

	/**
		Input mesh is scaled and transformed internally to fit unit cube centered in origin.
		Method provides offset vector and scale parameter;
	*/
	void									getTransformation(physx::PxVec3& offset, float& scale);


	/**
		Fractures specified chunk with voronoi method.
		\param[in] chunkId				Chunk to fracture
		\param[in] cellPoints			Array of voronoi sites
		\param[in] replaceChunk			if 'true', newly generated chunks will replace source chunk, if 'false', newly generated chunks will be at next depth level, source chunk will be parent for them.
										Case replaceChunk == true && chunkId == 0 considered as wrong input parameters
		\return   If 0, fracturing is successful.
	*/
	int32_t									voronoiFracturing(uint32_t chunkId, const std::vector<physx::PxVec3>& cellPoints, bool replaceChunk);

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
	int32_t									voronoiFracturing(uint32_t chunkId, const std::vector<physx::PxVec3>& cellPoints, const physx::PxVec3& scale, bool replaceChunk);


	/**
		Fractures specified chunk with slicing method.
		\param[in] chunkId				Chunk to fracture
		\param[in] conf					Slicing parameters, see SlicingConfiguration.
		\param[in] replaceChunk			if 'true', newly generated chunks will replace source chunk, if 'false', newly generated chunks will be at next depth level, source chunk will be parent for them.
										Case replaceChunk == true && chunkId == 0 considered as wrong input parameters
		\param[in] rnd					User supplied random number generator

		\return   If 0, fracturing is successful.
	*/
	int32_t									slicing(uint32_t chunkId, SlicingConfiguration conf, bool replaceChunk, RandomGeneratorBase* rnd);


	/**
		Creates resulting fractured mesh geometry from intermediate format
	*/
	void									finalizeFracturing();
	
	/**
		Get chunk information
	*/
	const std::vector<ChunkInfo>&    		getChunkList();


	/**
		Tesselate interior surfaces
		\param[in] averageEdgeLength - Average length of edge on internal surface.
	*/
	void									tesselate(float averageEdgeLength);
	
	/**
		Apply noise to interior surfaces. Must be called only after tesselation!
		\param[in] amplitude Amplitude of noise
		\param[in] frequency Frequency of noise
		\param[in] octaves Number of noise octaves
		\param[in] falloff - damping of noise around of external surface
		\param[in] relaxIterations - number of smoothing iterations before applying noise
		\param[in] relaxFactor - amount of smoothing before applying noise.
		\param[in] seed Random seed value
	*/
	void									applyNoise(float amplitude, float frequency, int32_t octaves, float falloff, int32_t relaxIterations, float relaxFactor, int32_t seed = 0);

	/**
		Get percentage of mesh overlap.
		percentage computed as volume(intersection(meshA , meshB)) / volume (meshA) 
		\param[in] meshA Mesh A
		\param[in] meshB Mesh B
		\return mesh overlap percentage
	*/
	static float							getMeshOverlap(Mesh& meshA, Mesh& meshB);

	/**
		Get chunk base mesh
		\param[in] chunkIndex Chunk index
		\param[out] output Array of triangles to be filled
	*/
	void									getBaseMesh(int32_t chunkIndex, std::vector<Triangle>& output);

	/**
		Get chunk mesh with noise
		\param[in] chunkIndex Chunk index
		\param[out] output Array of triangles to be filled
	*/
	void									getNoisedMesh(int32_t chunkIndex, std::vector<Triangle>& output);


	/**
		Return index of chunk with specified chunkId
		\param[in] chunkId Chunk ID
		\return Chunk index in internal buffer, if not exist -1 is returned.
	*/
	int32_t									getChunkIndex(int32_t chunkId);

	/**
		Return id of chunk with specified index.
		\param[in] chunkIndex Chunk index
		\return Chunk id or -1 if there is no such chunk.
	*/
	int32_t									getChunkId(int32_t chunkIndex);

	/**
		Return depth level of the given chunk
		\param[in] chunkId Chunk ID
		\return Chunk depth or -1 if there is no such chunk.
	*/
	int32_t									getChunkDepth(int32_t chunkId);

	/**
		Return array of chunks IDs with given depth.
		\param[in] depth Chunk depth
		\return Array of chunk IDs
	*/
	std::vector<int32_t>					getChunksIdAtDepth(uint32_t depth);


	/**
		Get result geometry without noise as vertex and index buffers, where index buffers contain series of triplets
		which represent triangles.
		\param[out] vertexBuffer Array of vertices to be filled
		\param[out] indexBuffer Array of arrays of indices to be filled
	*/
	void									getBufferedBaseMeshes(std::vector<Vertex>& vertexBuffer, std::vector<std::vector<uint32_t> >& indexBuffer);

	/**
		Get result geometry after tesselation and application of noise as vertex and index buffers, where index buffers contain series of triplets
		which represent triangles.
		\param[out] vertexBuffer Array of vertices to be filled
		\param[out] indexBuffer Array of arrays of indices to be filled
	*/
	void									getBufferedNoiseMeshes(std::vector<Vertex>& vertexBuffer, std::vector<std::vector<uint32_t> >& indexBuffer);

	/**
		Set automatic islands removing. May cause instabilities.
		\param[in] isRemoveIslands Flag whether remove or not islands.
	*/
	void									setRemoveIslands(bool isRemoveIslands);

	/**
		Try find islands and remove them on some specifical chunk. If chunk has childs, island removing can lead to wrong results! Apply it before further chunk splitting.
		\param[in] chunkId Chunk ID which should be checked for islands
		\return Number of found islands is returned
	*/
	int32_t									islandDetectionAndRemoving(int32_t chunkId);

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
	std::vector<ChunkPostProcessor*>	mChunkPostprocessors;


	
	int32_t								mPlaneIndexerOffset;
	int32_t								mChunkIdCounter;
	std::vector<ChunkInfo>				mChunkData;

	bool								mRemoveIslands;

	NvBlastLog							mLoggingCallback;
};

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTAUTHORINGFRACTURETOOL_H
