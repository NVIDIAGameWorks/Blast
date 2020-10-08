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


#ifndef NVBLASTFAMILY_H
#define NVBLASTFAMILY_H


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

	/**
	Retrieve the actor from an index. If actor is inactive nullptr is returned.

	\param[in] index	The index of an actor.

	\return	A pointer to the indexed actor if the actor is active, nullptr otherwise.
	*/
	Actor*		getActorByIndex(uint32_t index) const;

	/**
	Retrieve the index of an actor associated with the given chunk.

	\param[in] chunkIndex	The index of chunk.

	\return the index of associated actor in the FamilyHeader's getActors() array.
	*/
	uint32_t	getGetChunkActorIndex(uint32_t chunkIndex) const;

	/**
	Retrieve the index of an actor associated with the given node.

	\param[in] nodeIndex	The index of node.

	\return the index of associated actor in the FamilyHeader's getActors() array.
	*/
	uint32_t	getGetNodeActorIndex(uint32_t nodeIndex) const;

	/**
	Retrieve an actor associated with the given chunk.

	\param[in] chunkIndex	The index of chunk.

	\return	A pointer to the actor if the actor is active, nullptr otherwise.
	*/
	Actor*		getGetChunkActor(uint32_t chunkIndex) const;

	/**
	Retrieve an actor associated with the given node.

	\param[in] nodeIndex	The index of node.

	\return	A pointer to the actor if the actor is active, nullptr otherwise.
	*/
	Actor*		getGetNodeActor(uint32_t nodeIndex) const;


	//////// Fracturing methods ////////

	/**
	Hierarchically distribute damage to child chunks.

	\param chunkIndex		asset chunk index to hierarchically damage
	\param suboffset		index of the first sub-support health
	\param healthDamage		damage strength to apply
	\param chunkHealths		instance chunk healths
	\param chunks			asset chunk collection
	*/
	void				fractureSubSupportNoEvents(uint32_t chunkIndex, uint32_t suboffset, float healthDamage, float* chunkHealths, const NvBlastChunk* chunks);

	/**
	Hierarchically distribute damage to child chunks, recording a fracture event for each health damage applied.

	If outBuffer is too small, events are dropped but the chunks are still damaged.

	\param chunkIndex		asset chunk index to hierarchically damage
	\param suboffset		index of the first sub-support health
	\param healthDamage		damage strength to apply
	\param chunkHealths		instance chunk healths
	\param chunks			asset chunk collection
	\param outBuffer		target buffer for fracture events
	\param currentIndex		current position in outBuffer - returns the number of damaged chunks
	\param maxCount			capacity of outBuffer
	\param[in] filterActor	pointer to the actor to filter commands that target other actors. May be NULL to apply all commands
	\param[in] logFn		User-supplied message function (see NvBlastLog definition).  May be NULL.
	*/
	void				fractureSubSupport(uint32_t chunkIndex, uint32_t suboffset, float healthDamage, float* chunkHealths, const NvBlastChunk* chunks, NvBlastChunkFractureData* outBuffer, uint32_t* currentIndex, const uint32_t maxCount);

	/**
	Apply chunk fracture commands hierarchically.

	\param chunkFractureCount	number of chunk fracture commands to apply
	\param chunkFractures		array of chunk fracture commands
	\param filterActor			pointer to the actor to filter commands corresponding to other actors. May be NULL to apply all commands
	\param[in] logFn			User-supplied message function (see NvBlastLog definition).  May be NULL.
	*/
	void				fractureNoEvents(uint32_t chunkFractureCount, const NvBlastChunkFractureData* chunkFractures, Actor* filterActor, NvBlastLog logFn);

	/**
	Apply chunk fracture commands hierarchically, recording a fracture event for each health damage applied.

	If events array is too small, events are dropped but the chunks are still damaged.

	\param chunkFractureCount	number of chunk fracture commands to apply
	\param commands				array of chunk fracture commands
	\param events				target buffer for fracture events
	\param eventsSize			number of available entries in 'events'
	\param count				returns the number of damaged chunks
	\param[in] filterActor		pointer to the actor to filter commands that target other actors. May be NULL to apply all commands
	\param[in] logFn			User-supplied message function (see NvBlastLog definition).  May be NULL.

	*/
	void				fractureWithEvents(uint32_t chunkFractureCount, const NvBlastChunkFractureData* commands, NvBlastChunkFractureData* events, uint32_t eventsSize, uint32_t* count, Actor* filterActor, NvBlastLog logFn);

	/**
	Apply chunk fracture commands hierarchically, recording a fracture event for each health damage applied.

	In-Place version: fracture commands are replaced by fracture events.

	If inoutbuffer array is too small, events are dropped but the chunks are still damaged.

	\param chunkFractureCount	number of chunk fracture commands to apply
	\param inoutbuffer			array of chunk fracture commands to be replaced by events
	\param eventsSize			number of available entries in inoutbuffer
	\param count				returns the number of damaged chunks
	\param[in] filterActor		pointer to the actor to filter commands that target other actors. May be NULL to apply all commands
	\param[in] logFn			User-supplied message function (see NvBlastLog definition).  May be NULL.

	*/
	void				fractureInPlaceEvents(uint32_t chunkFractureCount, NvBlastChunkFractureData* inoutbuffer, uint32_t eventsSize, uint32_t* count, Actor* filterActor, NvBlastLog logFn);

	/**
	See NvBlastActorApplyFracture

	\param[in,out]	eventBuffers		Target buffers to hold applied fracture events. May be NULL, in which case events are not reported.
	To avoid data loss, provide an entry for every lower-support chunk and every bond in the original actor.
	\param[in,out]	actor				The NvBlastActor to apply fracture to.
	\param[in]		commands			The fracture commands to process.
	\param[in]		filterActor			pointer to the actor to filter commands that target other actors. May be NULL to apply all commands
	\param[in]		logFn				User-supplied message function (see NvBlastLog definition).  May be NULL.
	\param[in,out]	timers				If non-NULL this struct will be filled out with profiling information for the step, in profile build configurations.
	*/
	void				applyFracture(NvBlastFractureBuffers* eventBuffers, const NvBlastFractureBuffers* commands, Actor* filterActor, NvBlastLog logFn, NvBlastTimers* timers);
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


NV_INLINE Actor* FamilyHeader::getActorByIndex(uint32_t index) const
{
	NVBLAST_ASSERT(index < getActorBufferSize());
	Actor& actor = getActors()[index];
	return actor.isActive() ? &actor : nullptr;
}


NV_INLINE uint32_t FamilyHeader::getGetChunkActorIndex(uint32_t chunkIndex) const
{
	NVBLAST_ASSERT(m_asset);
	NVBLAST_ASSERT(chunkIndex < m_asset->m_chunkCount);
	if (chunkIndex < m_asset->getUpperSupportChunkCount())
	{
		return getChunkActorIndices()[chunkIndex];
	}
	else
	{
		return chunkIndex - (m_asset->getUpperSupportChunkCount() - m_asset->m_graph.m_nodeCount);
	}
}


NV_INLINE uint32_t FamilyHeader::getGetNodeActorIndex(uint32_t nodeIndex) const
{
	NVBLAST_ASSERT(m_asset);
	NVBLAST_ASSERT(nodeIndex < m_asset->m_graph.m_nodeCount);
	const uint32_t chunkIndex = m_asset->m_graph.getChunkIndices()[nodeIndex];
	return isInvalidIndex(chunkIndex) ? chunkIndex : getChunkActorIndices()[chunkIndex];
}


NV_INLINE Actor* FamilyHeader::getGetChunkActor(uint32_t chunkIndex) const
{
	uint32_t actorIndex = getGetChunkActorIndex(chunkIndex);
	return !isInvalidIndex(actorIndex) ? getActorByIndex(actorIndex) : nullptr;
}


NV_INLINE Actor* FamilyHeader::getGetNodeActor(uint32_t nodeIndex) const
{
	uint32_t actorIndex = getGetNodeActorIndex(nodeIndex);
	return !isInvalidIndex(actorIndex) ? getActorByIndex(actorIndex) : nullptr;
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
