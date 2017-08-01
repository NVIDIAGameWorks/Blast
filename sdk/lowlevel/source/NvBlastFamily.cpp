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
// Copyright (c) 2016-2017 NVIDIA Corporation. All rights reserved.


#include "NvBlastFamily.h"
#include "NvBlastFamilyGraph.h"
#include "NvBlastIndexFns.h"
#include "NvBlastTime.h"

#include <new>

namespace Nv
{
namespace Blast
{

//////// Global functions ////////

struct FamilyDataOffsets
{
	size_t m_actors;
	size_t m_visibleChunkIndexLinks;
	size_t m_chunkActorIndices;
	size_t m_graphNodeIndexLinks;
	size_t m_lowerSupportChunkHealths;
	size_t m_graphBondHealths;
	size_t m_familyGraph;
};


static size_t createFamilyDataOffsets(FamilyDataOffsets& offsets, const Asset* asset)
{
	const SupportGraph& graph = asset->m_graph;

	NvBlastCreateOffsetStart(sizeof(FamilyHeader));
	NvBlastCreateOffsetAlign16(offsets.m_actors, asset->getLowerSupportChunkCount() * sizeof(Actor));
	NvBlastCreateOffsetAlign16(offsets.m_visibleChunkIndexLinks, asset->m_chunkCount * sizeof(IndexDLink<uint32_t>));
	NvBlastCreateOffsetAlign16(offsets.m_chunkActorIndices, asset->getUpperSupportChunkCount() * sizeof(uint32_t));
	NvBlastCreateOffsetAlign16(offsets.m_graphNodeIndexLinks, graph.m_nodeCount * sizeof(uint32_t));
	NvBlastCreateOffsetAlign16(offsets.m_lowerSupportChunkHealths, asset->getLowerSupportChunkCount() * sizeof(float));
	NvBlastCreateOffsetAlign16(offsets.m_graphBondHealths, asset->getBondCount() * sizeof(float));
	NvBlastCreateOffsetAlign16(offsets.m_familyGraph, static_cast<size_t>(FamilyGraph::requiredMemorySize(graph.m_nodeCount, asset->getBondCount())));
	return NvBlastCreateOffsetEndAlign16();
}


size_t getFamilyMemorySize(const Asset* asset)
{
#if NVBLASTLL_CHECK_PARAMS
	if (asset == nullptr)
	{
		NVBLAST_ALWAYS_ASSERT();
		return 0;
	}
#endif

	FamilyDataOffsets offsets;
	return createFamilyDataOffsets(offsets, asset);
}


NvBlastFamily* createFamily(void* mem, const NvBlastAsset* asset, NvBlastLog logFn)
{
	NVBLASTLL_CHECK(mem != nullptr, logFn, "createFamily: NULL mem pointer input.", return nullptr);
	NVBLASTLL_CHECK(asset != nullptr, logFn, "createFamily: NULL asset pointer input.", return nullptr);

	NVBLASTLL_CHECK((reinterpret_cast<uintptr_t>(mem) & 0xF) == 0, logFn, "createFamily: mem pointer not 16-byte aligned.", return nullptr);

	const Asset& solverAsset = *static_cast<const Asset*>(asset);

	if (solverAsset.m_chunkCount == 0)
	{
		NVBLASTLL_LOG_ERROR(logFn, "createFamily: Asset has no chunks.  Family not created.\n");
		return nullptr;
	}

	const SupportGraph& graph = solverAsset.m_graph;

	const uint32_t bondCount = solverAsset.getBondCount();

	// We need to keep this many actor representations around for our island indexing scheme.
	const uint32_t lowerSupportChunkCount = solverAsset.getLowerSupportChunkCount();

	// We need this many chunk actor indices.
	const uint32_t upperSupportChunkCount = solverAsset.getUpperSupportChunkCount();

	// Family offsets
	FamilyDataOffsets offsets;
	const size_t dataSize = createFamilyDataOffsets(offsets, &solverAsset);

	// Restricting our data size to < 4GB so that we may use uint32_t offsets
	if (dataSize > (size_t)UINT32_MAX)
	{
		NVBLASTLL_LOG_ERROR(logFn, "Nv::Blast::Actor::instanceAllocate: Instance data block size will exceed 4GB.  Instance not created.\n");
		return nullptr;
	}

	// Allocate family
	NvBlastFamily* family = (NvBlastFamily*)mem;

	// Fill in family header
	FamilyHeader* header = (FamilyHeader*)family;
	header->dataType = NvBlastDataBlock::FamilyDataBlock;
	header->formatVersion = 0;	// Not currently using this field
	header->size = (uint32_t)dataSize;
	header->m_assetID = solverAsset.m_ID;
	header->m_actorCount = 0;
	header->m_actorsOffset = (uint32_t)offsets.m_actors;
	header->m_visibleChunkIndexLinksOffset = (uint32_t)offsets.m_visibleChunkIndexLinks;
	header->m_chunkActorIndicesOffset = (uint32_t)offsets.m_chunkActorIndices;
	header->m_graphNodeIndexLinksOffset = (uint32_t)offsets.m_graphNodeIndexLinks;
	header->m_lowerSupportChunkHealthsOffset = (uint32_t)offsets.m_lowerSupportChunkHealths;
	header->m_graphBondHealthsOffset = (uint32_t)offsets.m_graphBondHealths;
	header->m_familyGraphOffset = (uint32_t)offsets.m_familyGraph;

	// Runtime data
	header->m_asset = &solverAsset;	// NOTE: this should be resolved from m_assetID

	// Initialize family header data:

	// Actors - initialize to defaults, with zero offset value (indicating inactive state)
	Actor* actors = header->getActors();	// This will get the subsupport actors too
	for (uint32_t i = 0; i < lowerSupportChunkCount; ++i)
	{
		new (actors + i) Actor();
	}

	// Visible chunk index links - initialize to solitary links (0xFFFFFFFF fields)
	memset(header->getVisibleChunkIndexLinks(), 0xFF, solverAsset.m_chunkCount*sizeof(IndexDLink<uint32_t>));

	// Chunk actor IDs - initialize to invalid (0xFFFFFFFF)
	memset(header->getChunkActorIndices(), 0xFF, upperSupportChunkCount*sizeof(uint32_t));

	// Graph node index links - initialize to solitary links
	memset(header->getGraphNodeIndexLinks(), 0xFF, graph.m_nodeCount*sizeof(uint32_t));

	// Healths are initialized to 0
	memset(header->getLowerSupportChunkHealths(), 0, lowerSupportChunkCount*sizeof(float));
	memset(header->getBondHealths(), 0, bondCount*sizeof(float));

	// FamilyGraph ctor
	new (header->getFamilyGraph()) FamilyGraph(&graph);

	return family;
}


//////// Family member methods ////////

void FamilyHeader::fractureSubSupportNoEvents(uint32_t chunkIndex, uint32_t suboffset, float healthDamage, float* chunkHealths, const NvBlastChunk* chunks)
{
	const NvBlastChunk& chunk = chunks[chunkIndex];
	uint32_t numChildren = chunk.childIndexStop - chunk.firstChildIndex;

	if (numChildren > 0)
	{
		healthDamage /= numChildren;
		for (uint32_t childIndex = chunk.firstChildIndex; childIndex < chunk.childIndexStop; childIndex++)
		{
			float& health = chunkHealths[childIndex - suboffset];
			if (health > 0.0f)
			{
				float remainingDamage = healthDamage - health;
				health -= healthDamage;

				NVBLAST_ASSERT(chunks[childIndex].parentChunkIndex == chunkIndex);

				if (health <= 0.0f && remainingDamage > 0.0f)
				{
					fractureSubSupportNoEvents(childIndex, suboffset, remainingDamage, chunkHealths, chunks);
				}
			}
		}
	}
}


void FamilyHeader::fractureSubSupport(uint32_t chunkIndex, uint32_t suboffset, float healthDamage, float* chunkHealths, const NvBlastChunk* chunks, NvBlastChunkFractureData* outBuffer, uint32_t* currentIndex, const uint32_t maxCount)
{
	const NvBlastChunk& chunk = chunks[chunkIndex];
	uint32_t numChildren = chunk.childIndexStop - chunk.firstChildIndex;

	if (numChildren > 0)
	{
		healthDamage /= numChildren;
		for (uint32_t childIndex = chunk.firstChildIndex; childIndex < chunk.childIndexStop; childIndex++)
		{
			float& health = chunkHealths[childIndex - suboffset];
			if (health > 0.0f)
			{
				float remainingDamage = healthDamage - health;
				health -= healthDamage;

				NVBLAST_ASSERT(chunks[childIndex].parentChunkIndex == chunkIndex);

				if (*currentIndex < maxCount)
				{
					NvBlastChunkFractureData& event = outBuffer[*currentIndex];
					event.userdata = chunks[childIndex].userData;
					event.chunkIndex = childIndex;
					event.health = health;
				}
				(*currentIndex)++;

				if (health <= 0.0f && remainingDamage > 0.0f)
				{
					fractureSubSupport(childIndex, suboffset, remainingDamage, chunkHealths, chunks, outBuffer, currentIndex, maxCount);
				}
			}
		}
	}

}


void FamilyHeader::fractureNoEvents(uint32_t chunkFractureCount, const NvBlastChunkFractureData* chunkFractures, Actor* filterActor, NvBlastLog logFn)
{
	const SupportGraph& graph = m_asset->m_graph;
	const uint32_t* graphAdjacencyPartition = graph.getAdjacencyPartition();
	const uint32_t* adjacentBondIndices = graph.getAdjacentBondIndices();
	float* bondHealths = getBondHealths();
	float* chunkHealths = getLowerSupportChunkHealths();
	float* subChunkHealths = getSubsupportChunkHealths();
	const NvBlastChunk* chunks = m_asset->getChunks();

	for (uint32_t i = 0; i < chunkFractureCount; ++i)
	{
		const NvBlastChunkFractureData& command = chunkFractures[i];
		const uint32_t chunkIndex = command.chunkIndex;
		const uint32_t chunkHealthIndex = m_asset->getContiguousLowerSupportIndex(chunkIndex);
		NVBLAST_ASSERT(!isInvalidIndex(chunkHealthIndex));
		if (isInvalidIndex(chunkHealthIndex))
		{
			continue;
		}
		float& health = chunkHealths[chunkHealthIndex];
		if (health > 0.0f && command.health > 0.0f)
		{
			Actor* actor = getGetChunkActor(chunkIndex);
			if (filterActor && filterActor != actor)
			{
				NVBLASTLL_LOG_WARNING(logFn, "NvBlastActorApplyFracture: chunk fracture command corresponds to other actor, command is ignored.");
			}
			else if (actor)
			{
				const uint32_t nodeIndex = m_asset->getChunkToGraphNodeMap()[chunkIndex];
				if (actor->getGraphNodeCount() > 1 && !isInvalidIndex(nodeIndex))
				{
					for (uint32_t adjacentIndex = graphAdjacencyPartition[nodeIndex]; adjacentIndex < graphAdjacencyPartition[nodeIndex + 1]; adjacentIndex++)
					{
						const uint32_t bondIndex = adjacentBondIndices[adjacentIndex];
						NVBLAST_ASSERT(!isInvalidIndex(bondIndex));
						if (bondHealths[bondIndex] > 0.0f)
						{
							bondHealths[bondIndex] = 0.0f;
						}
					}
					getFamilyGraph()->notifyNodeRemoved(actor->getIndex(), nodeIndex, &graph);
				}

				health -= command.health;

				const float remainingDamage = -health;

				if (remainingDamage > 0.0f) // node chunk has been damaged beyond its health
				{
					fractureSubSupportNoEvents(chunkIndex, m_asset->m_firstSubsupportChunkIndex, remainingDamage, subChunkHealths, chunks);
				}
			}
		}
	}
}


void FamilyHeader::fractureWithEvents(uint32_t chunkFractureCount, const NvBlastChunkFractureData* commands, NvBlastChunkFractureData* events, uint32_t eventsSize, uint32_t* count, Actor* filterActor, NvBlastLog logFn)
{
	const SupportGraph& graph = m_asset->m_graph;
	const uint32_t* graphAdjacencyPartition = graph.getAdjacencyPartition();
	const uint32_t* adjacentBondIndices = graph.getAdjacentBondIndices();
	float* bondHealths = getBondHealths();
	float* chunkHealths = getLowerSupportChunkHealths();
	float* subChunkHealths = getSubsupportChunkHealths();
	const NvBlastChunk* chunks = m_asset->getChunks();

	for (uint32_t i = 0; i < chunkFractureCount; ++i)
	{
		const NvBlastChunkFractureData& command = commands[i];
		const uint32_t chunkIndex = command.chunkIndex;
		const uint32_t chunkHealthIndex = m_asset->getContiguousLowerSupportIndex(chunkIndex);
		NVBLAST_ASSERT(!isInvalidIndex(chunkHealthIndex));
		if (isInvalidIndex(chunkHealthIndex))
		{
			continue;
		}
		float& health = chunkHealths[chunkHealthIndex];
		if (health > 0.0f && command.health > 0.0f)
		{
			Actor* actor = getGetChunkActor(chunkIndex);
			if (filterActor && filterActor != actor)
			{
				NVBLASTLL_LOG_WARNING(logFn, "NvBlastActorApplyFracture: chunk fracture command corresponds to other actor, command is ignored.");
			}
			else if (actor)
			{
				const uint32_t nodeIndex = m_asset->getChunkToGraphNodeMap()[chunkIndex];
				if (actor->getGraphNodeCount() > 1 && !isInvalidIndex(nodeIndex))
				{
					for (uint32_t adjacentIndex = graphAdjacencyPartition[nodeIndex]; adjacentIndex < graphAdjacencyPartition[nodeIndex + 1]; adjacentIndex++)
					{
						const uint32_t bondIndex = adjacentBondIndices[adjacentIndex];
						NVBLAST_ASSERT(!isInvalidIndex(bondIndex));
						if (bondHealths[bondIndex] > 0.0f)
						{
							bondHealths[bondIndex] = 0.0f;
						}
					}
					getFamilyGraph()->notifyNodeRemoved(actor->getIndex(), nodeIndex, &graph);
				}

				health -= command.health;

				if (*count < eventsSize)
				{
					NvBlastChunkFractureData& outEvent = events[*count];
					outEvent.userdata = chunks[chunkIndex].userData;
					outEvent.chunkIndex = chunkIndex;
					outEvent.health = health;
				}
				(*count)++;

				const float remainingDamage = -health;

				if (remainingDamage > 0.0f) // node chunk has been damaged beyond its health
				{
					fractureSubSupport(chunkIndex, m_asset->m_firstSubsupportChunkIndex, remainingDamage, subChunkHealths, chunks, events, count, eventsSize);
				}
			}
		}
	}
}


void FamilyHeader::fractureInPlaceEvents(uint32_t chunkFractureCount, NvBlastChunkFractureData* inoutbuffer, uint32_t eventsSize, uint32_t* count, Actor* filterActor, NvBlastLog logFn)
{
	const SupportGraph& graph = m_asset->m_graph;
	const uint32_t* graphAdjacencyPartition = graph.getAdjacencyPartition();
	const uint32_t* adjacentBondIndices = graph.getAdjacentBondIndices();
	float* bondHealths = getBondHealths();
	float* chunkHealths = getLowerSupportChunkHealths();
	float* subChunkHealths = getSubsupportChunkHealths();
	const NvBlastChunk* chunks = m_asset->getChunks();

	//
	// First level Chunk Fractures
	//

	for (uint32_t i = 0; i < chunkFractureCount; ++i)
	{
		const NvBlastChunkFractureData& command = inoutbuffer[i];
		const uint32_t chunkIndex = command.chunkIndex;
		const uint32_t chunkHealthIndex = m_asset->getContiguousLowerSupportIndex(chunkIndex);
		NVBLAST_ASSERT(!isInvalidIndex(chunkHealthIndex));
		if (isInvalidIndex(chunkHealthIndex))
		{
			continue;
		}
		float& health = chunkHealths[chunkHealthIndex];
		if (health > 0.0f && command.health > 0.0f)
		{
			Actor* actor = getGetChunkActor(chunkIndex);
			if (filterActor && filterActor != actor)
			{
				NVBLASTLL_LOG_WARNING(logFn, "NvBlastActorApplyFracture: chunk fracture command corresponds to other actor, command is ignored.");
			}
			else if (actor)
			{
				const uint32_t nodeIndex = m_asset->getChunkToGraphNodeMap()[chunkIndex];
				if (actor->getGraphNodeCount() > 1 && !isInvalidIndex(nodeIndex))
				{
					for (uint32_t adjacentIndex = graphAdjacencyPartition[nodeIndex]; adjacentIndex < graphAdjacencyPartition[nodeIndex + 1]; adjacentIndex++)
					{
						const uint32_t bondIndex = adjacentBondIndices[adjacentIndex];
						NVBLAST_ASSERT(!isInvalidIndex(bondIndex));
						if (bondHealths[bondIndex] > 0.0f)
						{
							bondHealths[bondIndex] = 0.0f;
						}
					}
					getFamilyGraph()->notifyNodeRemoved(actor->getIndex(), nodeIndex, &graph);
				}

				health -= command.health;

				NvBlastChunkFractureData& outEvent = inoutbuffer[(*count)++];
				outEvent.userdata = chunks[chunkIndex].userData;
				outEvent.chunkIndex = chunkIndex;
				outEvent.health = health;
			}
		}
	}

	//
	// Hierarchical Chunk Fractures
	//

	uint32_t commandedChunkFractures = *count;

	for (uint32_t i = 0; i < commandedChunkFractures; ++i)
	{
		NvBlastChunkFractureData& event = inoutbuffer[i];
		const uint32_t chunkIndex = event.chunkIndex;

		const float remainingDamage = -event.health;
		if (remainingDamage > 0.0f) // node chunk has been damaged beyond its health
		{
			fractureSubSupport(chunkIndex, m_asset->m_firstSubsupportChunkIndex, remainingDamage, subChunkHealths, chunks, inoutbuffer, count, eventsSize);
		}
	}
}


void FamilyHeader::applyFracture(NvBlastFractureBuffers* eventBuffers, const NvBlastFractureBuffers* commands, Actor* filterActor, NvBlastLog logFn, NvBlastTimers* timers)
{
	NVBLASTLL_CHECK(commands != nullptr, logFn, "NvBlastActorApplyFracture: NULL commands pointer input.", return);
	NVBLASTLL_CHECK(isValid(commands), logFn, "NvBlastActorApplyFracture: commands memory is NULL but size is > 0.", return);
	NVBLASTLL_CHECK(eventBuffers == nullptr || isValid(eventBuffers), logFn, "NvBlastActorApplyFracture: eventBuffers memory is NULL but size is > 0.",
		eventBuffers->bondFractureCount = 0; eventBuffers->chunkFractureCount = 0; return);

#if NVBLASTLL_CHECK_PARAMS
	if (eventBuffers != nullptr && eventBuffers->bondFractureCount == 0 && eventBuffers->chunkFractureCount == 0)
	{
		NVBLASTLL_LOG_WARNING(logFn, "NvBlastActorApplyFracture: eventBuffers do not provide any space.");
		return;
	}
#endif

#if NV_PROFILE
	Time time;
#else
	NV_UNUSED(timers);
#endif

	//
	// Chunk Fracture
	//

	if (eventBuffers == nullptr || eventBuffers->chunkFractures == nullptr)
	{
		// immediate hierarchical fracture
		fractureNoEvents(commands->chunkFractureCount, commands->chunkFractures, filterActor, logFn);
	}
	else if (eventBuffers->chunkFractures != commands->chunkFractures)
	{
		// immediate hierarchical fracture
		uint32_t count = 0;
		fractureWithEvents(commands->chunkFractureCount, commands->chunkFractures, eventBuffers->chunkFractures, eventBuffers->chunkFractureCount, &count, filterActor, logFn);

		if (count > eventBuffers->chunkFractureCount)
		{
			NVBLASTLL_LOG_WARNING(logFn, "NvBlastActorApplyFracture: eventBuffers too small. Chunk events were lost.");
		}
		else
		{
			eventBuffers->chunkFractureCount = count;
		}
	}
	else if (eventBuffers->chunkFractures == commands->chunkFractures)
	{
		// compacting first
		uint32_t count = 0;
		fractureInPlaceEvents(commands->chunkFractureCount, commands->chunkFractures, eventBuffers->chunkFractureCount, &count, filterActor, logFn);

		if (count > eventBuffers->chunkFractureCount)
		{
			NVBLASTLL_LOG_WARNING(logFn, "NvBlastActorApplyFracture: eventBuffers too small. Chunk events were lost.");
		}
		else
		{
			eventBuffers->chunkFractureCount = count;
		}
	}

	//
	// Bond Fracture
	//

	uint32_t outCount = 0;
	const uint32_t eventBufferSize = eventBuffers ? eventBuffers->bondFractureCount : 0;

	NvBlastBond* bonds = m_asset->getBonds();
	float* bondHealths = getBondHealths();
	const uint32_t* graphChunkIndices = m_asset->m_graph.getChunkIndices();
	for (uint32_t i = 0; i < commands->bondFractureCount; ++i)
	{
		const NvBlastBondFractureData& frac = commands->bondFractures[i];
		NVBLAST_ASSERT(frac.nodeIndex0 < m_asset->m_graph.m_nodeCount);
		NVBLAST_ASSERT(frac.nodeIndex1 < m_asset->m_graph.m_nodeCount);
		uint32_t chunkIndex0 = graphChunkIndices[frac.nodeIndex0];
		uint32_t chunkIndex1 = graphChunkIndices[frac.nodeIndex1];
		NVBLAST_ASSERT(!isInvalidIndex(chunkIndex0) || !isInvalidIndex(chunkIndex1));
		Actor* actor0 = !isInvalidIndex(chunkIndex0) ? getGetChunkActor(chunkIndex0) : nullptr;
		Actor* actor1 = !isInvalidIndex(chunkIndex1) ? getGetChunkActor(chunkIndex1) : nullptr;
		NVBLAST_ASSERT(actor0 != nullptr || actor1 != nullptr);
		// If actors are not nullptr and different then bond is already broken
		// One of actor can be nullptr which probably means it's 'world' node.
		if (actor0 == actor1 || actor0 == nullptr || actor1 == nullptr)
		{
			Actor* actor = actor0 ? actor0 : actor1;
			NVBLAST_ASSERT_WITH_MESSAGE(actor, "NvBlastActorApplyFracture: all actors in bond fracture command are nullptr, command will be safely ignored, but investigation is recommended.");
			if (filterActor && filterActor != actor)
			{
				NVBLASTLL_LOG_WARNING(logFn, "NvBlastActorApplyFracture: bond fracture command corresponds to other actor, command is ignored.");
			}
			else if (actor)
			{
				const uint32_t bondIndex = actor->damageBond(frac.nodeIndex0, frac.nodeIndex1, frac.health);
				if (!isInvalidIndex(bondIndex))
				{
					if (eventBuffers && eventBuffers->bondFractures)
					{
						if (outCount < eventBufferSize)
						{
							NvBlastBondFractureData& outEvent = eventBuffers->bondFractures[outCount];
							outEvent.userdata = bonds[bondIndex].userData;
							outEvent.nodeIndex0 = frac.nodeIndex0;
							outEvent.nodeIndex1 = frac.nodeIndex1;
							outEvent.health = bondHealths[bondIndex];
						}
					}
					outCount++;
				}
			}
		}
	}

	if (eventBuffers && eventBuffers->bondFractures)
	{
		if (outCount > eventBufferSize)
		{
			NVBLASTLL_LOG_WARNING(logFn, "NvBlastActorApplyFracture: eventBuffers too small. Bond events were lost.");
		}
		else
		{
			eventBuffers->bondFractureCount = outCount;
		}
	}

#if NV_PROFILE
	if (timers != nullptr)
	{
		timers->fracture += time.getElapsedTicks();
	}
#endif

}


} // namespace Blast
} // namespace Nv


// API implementation

extern "C"
{

NvBlastFamily* NvBlastAssetCreateFamily(void* mem, const NvBlastAsset* asset, NvBlastLog logFn)
{
	return Nv::Blast::createFamily(mem, asset, logFn);
}


uint32_t NvBlastFamilyGetFormatVersion(const NvBlastFamily* family, NvBlastLog logFn)
{
	NVBLASTLL_CHECK(family != nullptr, logFn, "NvBlastFamilyGetFormatVersion: NULL family pointer input.", return UINT32_MAX);
	return reinterpret_cast<const Nv::Blast::FamilyHeader*>(family)->formatVersion;
}


const NvBlastAsset* NvBlastFamilyGetAsset(const NvBlastFamily* family, NvBlastLog logFn)
{
	NVBLASTLL_CHECK(family != nullptr, logFn, "NvBlastFamilyGetAssetID: NULL family pointer input.", return nullptr);
	return reinterpret_cast<const Nv::Blast::FamilyHeader*>(family)->m_asset;
}


void NvBlastFamilySetAsset(NvBlastFamily* family, const NvBlastAsset* asset, NvBlastLog logFn)
{
	NVBLASTLL_CHECK(family != nullptr, logFn, "NvBlastFamilySetAsset: NULL family pointer input.", return);
	NVBLASTLL_CHECK(asset != nullptr, logFn, "NvBlastFamilySetAsset: NULL asset pointer input.", return);

	Nv::Blast::FamilyHeader* header = reinterpret_cast<Nv::Blast::FamilyHeader*>(family);
	const Nv::Blast::Asset* solverAsset = reinterpret_cast<const Nv::Blast::Asset*>(asset);

	if (memcmp(&header->m_assetID, &solverAsset->m_ID, sizeof(NvBlastID)))
	{
		NVBLASTLL_LOG_ERROR(logFn, "NvBlastFamilySetAsset: wrong asset.  Passed asset ID doesn't match family asset ID.");
		return;
	}

	header->m_asset = solverAsset;
}


uint32_t NvBlastFamilyGetSize(const NvBlastFamily* family, NvBlastLog logFn)
{
	NVBLASTLL_CHECK(family != nullptr, logFn, "NvBlastFamilyGetSize: NULL family pointer input.", return 0);
	return reinterpret_cast<const Nv::Blast::FamilyHeader*>(family)->size;
}


NvBlastID NvBlastFamilyGetAssetID(const NvBlastFamily* family, NvBlastLog logFn)
{
	NVBLASTLL_CHECK(family != nullptr, logFn, "NvBlastFamilyGetAssetID: NULL family pointer input.", return NvBlastID());
	return reinterpret_cast<const Nv::Blast::FamilyHeader*>(family)->m_assetID;
}


uint32_t NvBlastFamilyGetActorCount(const NvBlastFamily* family, NvBlastLog logFn)
{
	NVBLASTLL_CHECK(family != nullptr, logFn, "NvBlastFamilyGetActorCount: NULL family pointer input.", return 0);

	const Nv::Blast::FamilyHeader* header = reinterpret_cast<const Nv::Blast::FamilyHeader*>(family);

	return header->m_actorCount;
}


uint32_t NvBlastFamilyGetActors(NvBlastActor** actors, uint32_t actorsSize, const NvBlastFamily* family, NvBlastLog logFn)
{
	NVBLASTLL_CHECK(actors != nullptr, logFn, "NvBlastFamilyGetActors: NULL actors pointer input.", return 0);
	NVBLASTLL_CHECK(family != nullptr, logFn, "NvBlastFamilyGetActors: NULL family pointer input.", return 0);

	const Nv::Blast::FamilyHeader* header = reinterpret_cast<const Nv::Blast::FamilyHeader*>(family);

	// Iterate through active actors and write to supplied array
	const uint32_t familyActorCount = header->getActorBufferSize();
	Nv::Blast::Actor* familyActor = header->getActors();
	uint32_t actorCount = 0;
	for (uint32_t i = 0; actorCount < actorsSize && i < familyActorCount; ++i, ++familyActor)
	{
		if (familyActor->isActive())
		{
			actors[actorCount++] = familyActor;
		}
	}

	return actorCount;
}


NvBlastActor* NvBlastFamilyGetChunkActor(const NvBlastFamily* family, uint32_t chunkIndex, NvBlastLog logFn)
{
	NVBLASTLL_CHECK(family != nullptr, logFn, "NvBlastFamilyGetChunkActor: NULL family pointer input.", return nullptr);

	const Nv::Blast::FamilyHeader* header = reinterpret_cast<const Nv::Blast::FamilyHeader*>(family);

	NVBLASTLL_CHECK(header->m_asset != nullptr, logFn, "NvBlastFamilyGetChunkActor: NvBlastFamily has null asset set.", return nullptr);
	NVBLASTLL_CHECK(chunkIndex < header->m_asset->m_chunkCount, logFn, "NvBlastFamilyGetChunkActor: bad value of chunkIndex for the given family's asset.", return nullptr);

	return header->getGetChunkActor(chunkIndex);
}


uint32_t NvBlastFamilyGetMaxActorCount(const NvBlastFamily* family, NvBlastLog logFn)
{
	NVBLASTLL_CHECK(family != nullptr, logFn, "NvBlastFamilyGetMaxActorCount: NULL family pointer input.", return 0);
	const Nv::Blast::FamilyHeader* header = reinterpret_cast<const Nv::Blast::FamilyHeader*>(family);
	return header->getActorBufferSize();
}

} // extern "C"
