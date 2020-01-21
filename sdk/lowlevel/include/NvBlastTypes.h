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


#ifndef NVBLASTTYPES_H
#define NVBLASTTYPES_H


#include "NvBlastPreprocessor.h"
#include <stdint.h>


///////////////////////////////////////////////////////////////////////////////
//	NvBlast common types
///////////////////////////////////////////////////////////////////////////////
///@{

/**
Types of log messages.
*/
struct NvBlastMessage
{
	enum Type
	{
		Error,		//!< Error messages
		Warning,	//!< Warning messages
		Info,		//!< Information messages
		Debug		//!< Used only in debug version of dll
	};
};


/**
Function pointer type for logging.

When a function with this signature is passed into Blast functions with an NvBlastLog argument,
Blast will use it to report errors, warnings, and other information.
*/
typedef void(*NvBlastLog)(int type, const char* msg, const char* file, int line);


/**
ID used to identify assets.
*/
struct NvBlastID
{
	char data[16];
};


/**
Time spent (in ticks) in various parts of Blast.
These values may be filled in during the execution of various API functions.
To convert to seconds, use NvBlastTicksToSeconds(ticks).

In profile build configurations, if a pointer to an instance of this struct is passed into
Blast functions with an NvBlastTimers argument, then Blast will add to appropriate fields
the time measured in corresponding sections of code.  The user must clear the timer fields
with NvBlastTimersReset to initialize or reset.
*/
struct NvBlastTimers
{
	int64_t  material;		//!< Time spent in material function
	int64_t  fracture;		//!< Time spent applying damage
	int64_t  island;		//!< Time spent discovering islands
	int64_t  partition;		//!< Time spent partitioning the graph
	int64_t  visibility;	//!< Time spent updating visibility
};


/**
Generic data block header for all data blocks.
*/
struct NvBlastDataBlock
{
	/**
	Enum of data block types
	*/
	enum Type
	{
		AssetDataBlock,
		FamilyDataBlock,

		Count
	};


	/**
	A data type keeps value from Type enum
	*/
	uint32_t	dataType;

	/**
	A number which is incremented every time the data layout changes. Depending on dataType corresponding	data 
	format is kept. See NvBlastAssetDataFormat, NvBlastFamilyDataFormat enum.
	*/
	uint32_t	formatVersion;

	/**
	The size of the family, including this header.

	Memory sizes are restricted to 32-bit representable values.
	*/
	uint32_t	size;

	/**
	Reserved to be possibly used in future versions
	*/
	uint32_t	reserved;
};

///@} End NvBlast common types


///////////////////////////////////////////////////////////////////////////////
//	NvBlastAsset related types
///////////////////////////////////////////////////////////////////////////////
///@{

/**
Represents a piece of a destructible asset which may be realized as an entity with a physical and graphical component.

Chunks may form a hierarchical decomposition of the asset.  They contain parent and child chunk index information which
defines the hierarchy.  The parent and child chunk indices are their positions with the NvBlastAsset::chunks array.

Child chunk indices are contiguous, starting at firstChildIndex and ending with childIndexStop - 1.
*/
struct NvBlastChunk
{
	/**
	Central position for the chunk's volume
	*/
	float		centroid[3];

	/**
	Volume of the chunk
	*/
	float		volume;

	/**
	Index of parent (UINT32_MAX denotes no parent)
	*/
	uint32_t	parentChunkIndex;

	/**
	Index of first child
	*/
	uint32_t	firstChildIndex;

	/**
	Stop for child indices
	*/
	uint32_t	childIndexStop;

	/**
	Field for user to associate with external data
	*/
	uint32_t	userData;
};


/**
Represents the interface between two chunks.  At most one bond is created for a chunk pair.
*/
struct NvBlastBond
{
	/**
	Interface average normal
	*/
	float		normal[3];

	/**
	Area of interface
	*/
	float		area;

	/**
	Central position on the interface between chunks
	*/
	float		centroid[3];

	/**
	Extra data associated with bond, e.g. whether or not to create a joint
	*/
	uint32_t	userData;
};


/**
Describes the connectivity between support chunks via bonds.

Vertices in the support graph are termed "nodes," and represent particular chunks (NvBlastChunk) in an NvBlastAsset.
The indexing for nodes is not the same as that for chunks.  Only some chunks are represented by nodes in the graph,
and these chunks are called "support chunks."

Adjacent node indices and adjacent bond indices are stored for each node, and therefore each bond is represented twice in this graph,
going from node[i] -> node[j] and from node[j] -> node[i].  Therefore the size of the adjacentNodeIndices and adjacentBondIndices
arrays are twice the number of bonds stored in the corresponding NvBlastAsset. 

The graph is used as follows.  Given a NvBlastSupportGraph "graph" and node index i, (0 <= i < graph.nodeCount), one may find all
adjacent bonds and nodes using:

	// adj is the lookup value in graph.adjacentNodeIndices and graph.adjacentBondIndices
	for (uint32_t adj = graph.adjacencyPartition[i]; adj < graph.adjacencyPartition[i+1]; ++adj)
	{
		// An adjacent node:
		uint32_t adjacentNodeIndex = graph.adjacentNodeIndices[adj];
	
		// The corresponding bond (that connects node index i with node indexed adjacentNodeIndex:
		uint32_t adjacentBondIndex = graph.adjacentBondIndices[adj];
	}

For a graph node with index i, the corresponding asset chunk index is found using graph.chunkIndices[i].  The reverse mapping
(obtaining a graph node index from an asset chunk index) can be done using the

	NvBlastAssetGetChunkToGraphNodeMap(asset, logFn)

function.  See the documentation for its use.  The returned "node index" for a non-support chunk is the invalid value 0xFFFFFFFF.
*/
struct NvBlastSupportGraph
{
	/**
	Total number of nodes in the support graph.
	*/
	uint32_t	nodeCount;

	/**
	Indices of chunks represented by the nodes, an array of size nodeCount.
	*/
	uint32_t*	chunkIndices;

	/**
	Partitions both the adjacentNodeIndices and the adjacentBondIndices arrays into subsets corresponding to each node.
	The size of this array is nodeCount+1.
	For 0 <= i < nodeCount, adjacencyPartition[i] is the index of the first element in adjacentNodeIndices (or adjacentBondIndices) for nodes adjacent to the node with index i.
	adjacencyPartition[nodeCount] is the size of the adjacentNodeIndices and adjacentBondIndices arrays.
	This allows one to easily count the number of nodes adjacent to a node with index i, using adjacencyPartition[i+1] - adjacencyPartition[i].
	*/
	uint32_t*	adjacencyPartition;

	/**
	Array composed of subarrays holding the indices of nodes adjacent to a given node.  The subarrays may be accessed through the adjacencyPartition array.
	*/
	uint32_t*	adjacentNodeIndices;

	/**
	Array composed of subarrays holding the indices of bonds (NvBlastBond) for a given node.  The subarrays may be accessed through the adjacencyPartition array.
	*/
	uint32_t*	adjacentBondIndices;
};


/**
Asset (opaque)

Static destructible data, used to create actor families.

Pointer to this struct can be created with NvBlastCreateAsset.

The NvBlastAsset includes a ID which may be used to match it with physics and graphics data.
*/
struct NvBlastAsset {};


/**
Chunk descriptor used to build an asset.  See NvBlastAssetDesc.
*/
struct NvBlastChunkDesc
{
	enum Flags
	{
		NoFlags = 0,

		/** If this flag is set then the chunk will become a support chunk, unless an ancestor chunk is also marked as support. */
		SupportFlag = (1 << 0)
	};

	/** Central position in chunk. */
	float		centroid[3];

	/** Volume of chunk. */
	float		volume;

	/** Index of this chunk's parent.  If this is a root chunk, then this value must be UINT32_MAX. */
	uint32_t	parentChunkIndex;

	/** See Flags enum for possible flags. */
	uint32_t	flags;

	/** User-supplied data which will be accessible to the user in chunk fracture events. */
	uint32_t	userData;
};


/**
Chunk bond descriptor used to build an asset.  See NvBlastAssetDesc.
*/
struct NvBlastBondDesc
{
	/** Bond data (see NvBlastBond). */
	NvBlastBond	bond;

	/**
	The indices of the chunks linked by this bond.  They must be different support chunk indices.
	If one of the chunk indices is the invalid index (UINT32_MAX), then this will create a bond between
	the chunk indexed by the other index (which must be valid) and "the world."  Any actor containing
	this bond will cause the function NvBlastActorIsBoundToWorld to return true.
	*/
	uint32_t	chunkIndices[2];
};


/**
Asset descriptor, used to build an asset with NvBlastCreateAsset

A valid asset descriptor must have a non-zero chunkCount and valid chunkDescs.

The user may create an asset with no bonds (e.g. a single-chunk asset).  In this case bondCount should be
zero and bondDescs is ignored.
*/
struct NvBlastAssetDesc
{
	/** The number of chunk descriptors. */
	uint32_t				chunkCount;

	/** Array of chunk descriptors of size chunkCount. */
	const NvBlastChunkDesc*	chunkDescs;

	/** The number of bond descriptors. */
	uint32_t				bondCount;

	/** Array of bond descriptors of size bondCount. */
	const NvBlastBondDesc*	bondDescs;
};

///@} End NvBlastAsset related types


///////////////////////////////////////////////////////////////////////////////
//	NvBlastActor related types
///////////////////////////////////////////////////////////////////////////////
///@{

/**
Family (opaque)

a family can be created by the NvBlastAssetCreateFamily function and released 
by the NvBlastFamilyRelease.  Family is needed to create first actor.  All the following
actors which can be created with NvBlastActorSplit function (as a result of fracture) will share the same family
block.  NvBlastFamilyGetActorCount can be used to know if family can be safely released.
*/
struct NvBlastFamily {};


/**
Actor (opaque)

Actors can be generated by the NvBlastFamilyCreateFirstActor
and NvBlastActorSplit functions.  Opaque NvBlastActor pointers reference data within the family
generated during NvBlastFamilyCreateFirstActor, and represent the actor in all actor-related API
functions.
*/
struct NvBlastActor {};


/**
Actor descriptor, used to create an instance of an NvBlastAsset with NvBlastFamilyCreateFirstActor

See NvBlastFamilyCreateFirstActor.
*/
struct NvBlastActorDesc
{
	/**
	Initial health of all bonds, if initialBondHealths is NULL (see initialBondHealths).
	*/
	float			uniformInitialBondHealth;

	/**
	Initial bond healths.  If not NULL, this array must be of length NvBlastAssetGetChunkCount(asset, logFn).
	If NULL, uniformInitialBondHealth must be set.
	*/
	const float*	initialBondHealths;

	/**
	Initial health of all lower-support chunks, if initialSupportChunkHealths is NULL (see initialSupportChunkHealths).
	*/
	float			uniformInitialLowerSupportChunkHealth;

	/**
	Initial health of all support chunks.  If not NULL, this must be of length
	NvBlastAssetGetSupportChunkCount(asset, logFn).nodeCount. The elements in the initialSupportChunkHealth
	array will correspond to the chunk indices in the NvBlastAssetGetSupportGraph(asset, logFn).chunkIndices
	array.  Every descendent of a support chunk will have its health initialized to its ancestor support
	chunk's health, so this initializes all lower-support chunk healths.
	If NULL, uniformInitialLowerSupportChunkHealth must be set.
	*/
	const float*	initialSupportChunkHealths;
};

///@} End NvBlastActor related types


///////////////////////////////////////////////////////////////////////////////
//	Types used for damage and fracturing
///////////////////////////////////////////////////////////////////////////////
///@{


/**
Fracture Data for Chunks

Data interpretation varies depending on the function used. 
@see NvBlastActorGenerateFracture NvBlastActorApplyFracture NvBlastFractureBuffers
*/
struct NvBlastChunkFractureData
{
	uint32_t	userdata;	//!<	chunk's user data
	uint32_t	chunkIndex;	//!<	asset chunk index
	float		health;		//!<	health value (damage or remains)
};


/**
Fracture Data for Bonds

Data interpretation varies depending on the function used.
@see NvBlastActorGenerateFracture NvBlastActorApplyFracture NvBlastFractureBuffers
*/
struct NvBlastBondFractureData
{
	uint32_t	userdata;	//!<	bond's user data
	uint32_t	nodeIndex0;	//!<	graph node index of bond
	uint32_t	nodeIndex1;	//!<	pair graph node index of bond
	float		health;		//!<	health value (damage or remains)
};


/**
Memory to be used by fracture functions.

Used as input and output target.
@see NvBlastActorGenerateFracture NvBlastActorApplyFracture
*/
struct NvBlastFractureBuffers
{
	uint32_t					bondFractureCount;		//!<	available elements in bondFractures
	uint32_t					chunkFractureCount;		//!<	available elements in chunkFractures
	NvBlastBondFractureData*	bondFractures;			//!<	memory to be filled by fracture functions
	NvBlastChunkFractureData*	chunkFractures;			//!<	memory to be filled by fracture functions
};


/**
Description of a NvBlastActorSplit result.
This tells the user about changes in the actor, or creation of children.
*/
struct NvBlastActorSplitEvent
{
	NvBlastActor*	deletedActor;	//!<	deleted actor or nullptr if actor has not changed
	NvBlastActor**	newActors;		//!<	list of created actors
};


/**
A single actor's representation used by NvBlastGraphShaderFunction.
*/
struct NvBlastGraphShaderActor
{
	uint32_t			actorIndex;				//!<	Actor's index.
	uint32_t			graphNodeCount;			//!<	Actor's graph node count.
	uint32_t			assetNodeCount;			//!<	Asset node count.
	uint32_t			firstGraphNodeIndex;	//!<	Entry index for graphNodeIndexLinks
	const uint32_t*		graphNodeIndexLinks;	//!<	Linked index list of connected nodes.  Traversable with nextIndex = graphNodeIndexLinks[currentIndex], terminates with 0xFFFFFFFF.
	const uint32_t*		chunkIndices;			//!<	Graph's map from node index to support chunk index.
	const uint32_t*		adjacencyPartition;		//!<	See NvBlastSupportGraph::adjacencyPartition.
	const uint32_t*		adjacentNodeIndices;	//!<	See NvBlastSupportGraph::adjacentNodeIndices.
	const uint32_t*		adjacentBondIndices;	//!<	See NvBlastSupportGraph::adjacentBondIndices.
	const NvBlastBond*	assetBonds;				//!<	NvBlastBonds geometry in the NvBlastAsset.
	const NvBlastChunk*	assetChunks;			//!<	NvBlastChunks geometry in the NvBlastAsset.
	const float*		familyBondHealths;		//!<	Actual bond health values for broken bond detection.
	const float*		supportChunkHealths;	//!<	Actual chunk health values for dead chunk detection.
	const uint32_t*		nodeActorIndices;		//!<	Family's map from node index to actor index.
};


/**
A single actor's representation used by NvBlastSubgraphShaderFunction.
*/
struct NvBlastSubgraphShaderActor
{
	uint32_t			chunkIndex;		//!<	Index of chunk represented by this actor.
	const NvBlastChunk*	assetChunks;	//!<	NvBlastChunks geometry in the NvBlastAsset.
};


/**
Damage shader for actors with more then one node in support graph.

From a an input actor data (NvBlastGraphShaderActor) and user custom data (params),
creates a list of NvBlastFractureCommand to be applied to the respective NvBlastActor.

\param[in,out]	commandBuffers			The resulting health damage to apply.
										Typically requires an array of size (number of support chunks) + (number of bonds) of the processed asset
										but may depend on the actual implementation.
\param[in]		actor					The actor representation used for creating commands.
\param[in]		programParams			A set of parameters defined by the damage shader implementer.

Interpretation of NvBlastFractureBuffers:
As input:
Counters denote available entries for FractureData.
Chunk and Bond userdata are not used.
Health values are not used.

As output:
Counters denote valid entires in FractureData arrays.
Chunks and Bond userdata reflect the respective userdata set during asset initialization.
Health values denote how much damage is to be applied.

@see NvBlastFractureBuffers NvBlastGraphShaderActor
*/
typedef void(*NvBlastGraphShaderFunction)(NvBlastFractureBuffers* commandBuffers, const NvBlastGraphShaderActor* actor, const void* programParams);


/**
Damage shader for actors with single chunk.

From a an input actor data (NvBlastSubgraphShaderActor) and user custom data (params),
creates a list of NvBlastFractureCommand to be applied to the respective NvBlastActor.

\param[in,out]	commandBuffers			The resulting health damage to apply.
										Typically requires an array of size (number of support chunks) + (number of bonds) of the processed asset
										but may depend on the actual implementation.
\param[in]		actor					The actor representation used for creating commands.
\param[in]		programParams			A set of parameters defined by the damage shader implementer.

Interpretation of NvBlastFractureBuffers:
As input:
Counters denote available entries for FractureData.
Chunk and Bond userdata are not used.
Health values are not used.

As output:
Counters denote valid entires in FractureData arrays.
Chunks and Bond userdata reflect the respective userdata set during asset initialization.
Health values denote how much damage is to be applied.

@see NvBlastFractureBuffers NvBlastSubgraphShaderActor
*/
typedef void(*NvBlastSubgraphShaderFunction)(NvBlastFractureBuffers* commandBuffers, const NvBlastSubgraphShaderActor* actor, const void* programParams);


/**
Damage Program. 

Contains both graph and subgraph shader. When used on actor appropriate shader will be called.
Any shader can be nullptr to be skipped.

@see NvBlastGraphShaderFunction NvBlastSubgraphShaderFunction
*/
struct NvBlastDamageProgram
{
	NvBlastGraphShaderFunction		graphShaderFunction;
	NvBlastSubgraphShaderFunction	subgraphShaderFunction;
};


///@} End of types used for damage and fracturing


#endif // ifndef NVBLASTTYPES_H
