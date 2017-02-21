/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTFAMILY_H
#define NVBLASTFAMILY_H


#include "NvBlastPreprocessorInternal.h"
#include "NvBlastAsset.h"
#include "NvBlastPreprocessor.h"
#include "NvBlastDLink.h"
#include "NvBlastAtomic.h"
#include "NvBlastMemory.h"

#include <cstring>


struct NvBlastAsset;


namespace Nv
{
namespace Blast
{

// Forward declarations
class FamilyGraph;
class Actor;
class Asset;


/**
Data header at the beginning of every NvBlastActor family

The block address may be cast to a valid FamilyHeader pointer.
*/
struct FamilyHeader : public NvBlastDataBlock
{
	/**
	The ID for the asset.  This will be resolved into a pointer in the runtime data.
	*/
	NvBlastID	m_assetID;

	/**
	Actors, of type Actor.

	Actors with support chunks will use this array in the range [0, m_asset->m_graphNodeCount),
	while subsupport actors will be placed in the range [m_asset->m_graphNodeCount, getActorBufferSize()).
	*/
	NvBlastBlockArrayData(Actor, m_actorsOffset, getActors, m_asset->m_graph.m_nodeCount);

	/**
	Visible chunk index links, of type IndexDLink<uint32_t>.

	getVisibleChunkIndexLinks returns an array of size m_asset->m_chunkCount of IndexDLink<uint32_t> (see IndexDLink).
	*/
	NvBlastBlockArrayData(IndexDLink<uint32_t>, m_visibleChunkIndexLinksOffset, getVisibleChunkIndexLinks, m_asset->m_chunkCount);

	/**
	Chunk actor IDs, of type uint32_t.  These correspond to the ID of the actor which owns each chunk.  A value of invalidIndex<uint32_t>() indicates no owner.

	getChunkActorIndices returns an array of size m_asset->m_firstSubsupportChunkIndex.
	*/
	NvBlastBlockArrayData(uint32_t, m_chunkActorIndicesOffset, getChunkActorIndices, m_asset->m_firstSubsupportChunkIndex);

	/**
	Graph node index links, of type uint32_t.  The successor to index[i] is m_graphNodeIndexLinksOffset[i].  A value of invalidIndex<uint32_t>() indicates no successor.

	getGraphNodeIndexLinks returns an array of size m_asset->m_graphNodeCount.
	*/
	NvBlastBlockArrayData(uint32_t, m_graphNodeIndexLinksOffset, getGraphNodeIndexLinks, m_asset->m_graph.m_nodeCount);

	/**
	Health for each support chunk and subsupport chunk, of type float.

	To access support chunks, use the corresponding graph node index in the array returned by getLowerSupportChunkHealths.

	To access subsupport chunk healths, use getSubsupportChunkHealths (see documentation for details).
	*/
	NvBlastBlockArrayData(float, m_lowerSupportChunkHealthsOffset, getLowerSupportChunkHealths, m_asset->getLowerSupportChunkCount());

	/**
	Utility function to get the start of the subsupport chunk health array.

	To access a subsupport chunk health indexed by i, use getSubsupportChunkHealths()[i - m_asset->m_firstSubsupportChunkIndex]

	\return the array of health values associated with all descendants of support chunks.
	*/
	float*	getSubsupportChunkHealths() const
	{
		NVBLAST_ASSERT(m_asset != nullptr);
		return (float*)((uintptr_t)this + m_lowerSupportChunkHealthsOffset) + m_asset->m_graph.m_nodeCount;
	}

	/**
	Bond health for the interfaces between two chunks, of type float.  Since the bond is shared by two chunks, the same bond health is used for chunk[i] -> chunk[j] as for chunk[j] -> chunk[i].

	getBondHealths returns the array of healths associated with all bonds in the support graph.
	*/
	NvBlastBlockArrayData(float, m_graphBondHealthsOffset, getBondHealths, m_asset->getBondCount());

	/**
	The instance graph for islands searching, of type FamilyGraph.

	Return the dynamic data generated for the support graph.  (See FamilyGraph.)
	This is used to store current connectivity information based upon bond and chunk healths, as well as cached intermediate data for faster incremental updates.
	*/
	NvBlastBlockData(FamilyGraph, m_familyGraphOffset, getFamilyGraph);


	//////// Runtime data ////////

	/**
	The number of actors using this block.
	*/
	volatile uint32_t	m_actorCount;

	/**
	The asset corresponding to all actors in this family.
	This is runtime data and will be resolved from m_assetID.
	*/
	union
	{
		const Asset*	m_asset;
		uint64_t		m_runtimePlaceholder;	// Make sure we reserve enough room for an 8-byte pointer
	};


	//////// Functions ////////

	/**
	Gets an actor from the actor array and validates it if it is not already valid.  This increments the actor reference count.

	\param[in] index	The index of the actor to borrow.  Must be in the range [0, getActorBufferSize()).

	\return	A pointer to the indexed Actor.
	*/
	Actor*		borrowActor(uint32_t index);

	/**
	Invalidates the actor if it is not already invalid.  This decrements the actor reference count, but does not free this block when the count goes to zero.

	\param[in] actor	The actor to invalidate.
	*/
	void		returnActor(Actor& actor);

	/**
	Returns the total number of actors in the Actor buffer, active and inactive.

	\return the number of Actors in the actor buffer.  See borrowActor.
	*/
	uint32_t	getActorBufferSize() const;

	/**
	Returns a value to indicate whether or not the Actor with the given index is valid for use (active).

	\return	true iff the indexed actor is active.
	*/
	bool		isActorActive(uint32_t index) const;
};

} // namespace Blast
} // namespace Nv


#include "NvBlastActor.h"


namespace Nv
{
namespace Blast
{

//////// FamilyHeader inline methods ////////

NV_INLINE Actor* FamilyHeader::borrowActor(uint32_t index)
{
	NVBLAST_ASSERT(index < getActorBufferSize());
	Actor& actor = getActors()[index];
	if (actor.m_familyOffset == 0)
	{
		const uintptr_t offset = (uintptr_t)&actor - (uintptr_t)this;
		NVBLAST_ASSERT(offset <= UINT32_MAX);
		actor.m_familyOffset = (uint32_t)offset;
		atomicIncrement(reinterpret_cast<volatile int32_t*>(&m_actorCount));
	}
	return &actor;
}


NV_INLINE void FamilyHeader::returnActor(Actor& actor)
{
	if (actor.m_familyOffset != 0)
	{
		actor.m_familyOffset = 0;
		// The actor count should be positive since this actor was valid.  Check to be safe.
		NVBLAST_ASSERT(m_actorCount > 0);
		atomicDecrement(reinterpret_cast<volatile int32_t*>(&m_actorCount));
	}
}


NV_INLINE uint32_t FamilyHeader::getActorBufferSize() const
{
	NVBLAST_ASSERT(m_asset);
	return m_asset->getLowerSupportChunkCount();
}


NV_INLINE bool FamilyHeader::isActorActive(uint32_t index) const
{
	NVBLAST_ASSERT(index < getActorBufferSize());
	return getActors()[index].m_familyOffset != 0;
}


//////// Global functions ////////

/**
Returns the number of bytes of memory that a family created using the given asset will require.  A pointer
to a block of memory of at least this size must be passed in as the mem argument of createFamily.

\param[in] asset	The asset that will be passed into NvBlastAssetCreateFamily.
*/
size_t getFamilyMemorySize(const Asset* asset);

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTFAMILY_H
