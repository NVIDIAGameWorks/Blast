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


#include "NvBlastActor.h"
#include "NvBlastActorSerializationBlock.h"
#include "NvBlastFamilyGraph.h"

#include <algorithm>


namespace Nv
{
namespace Blast
{

//////// Actor static methods for serialization ////////

Actor* Actor::deserialize(NvBlastFamily* family, const void* buffer, NvBlastLog logFn)
{
	NVBLASTLL_CHECK(family != nullptr, logFn, "Actor::deserialize: NULL family pointer input.", return nullptr);

	const ActorSerializationHeader* serHeader = reinterpret_cast<const ActorSerializationHeader*>(buffer);
	if (serHeader->m_formatVersion != ActorSerializationFormat::Current)
	{
		NVBLASTLL_LOG_ERROR(logFn, "Actor::deserialize: wrong data format.  Serialization data must be converted to current version.");
		return nullptr;
	}

	FamilyHeader* header = reinterpret_cast<FamilyHeader*>(family);
	const Asset* asset = header->m_asset;
	const SupportGraph& graph = asset->m_graph;
	const uint32_t* graphChunkIndices = graph.getChunkIndices();
	const uint32_t* graphAdjacencyPartition = graph.getAdjacencyPartition();
	const uint32_t* graphAdjacentNodeIndices = graph.getAdjacentNodeIndices();
	const uint32_t* graphAdjacentBondIndices = graph.getAdjacentBondIndices();

	Actor* actor = nullptr;
	const uint32_t actorIndex = serHeader->m_index;

	if (serHeader->m_index < header->getActorBufferSize())
	{
		if (!header->getActors()[actorIndex].isActive())
		{
			actor = header->borrowActor(serHeader->m_index);
		}
	}

	if (actor == nullptr)
	{
		NVBLASTLL_LOG_ERROR(logFn, "Actor::deserialize: invalid actor index in serialized data.  Actor not created.");
		return nullptr;
	}

	// Commonly used data
	uint32_t* chunkActorIndices = header->getChunkActorIndices();
	FamilyGraph* familyGraph = header->getFamilyGraph();

#if NVBLASTLL_CHECK_PARAMS
	{
		const uint32_t* serVisibleChunkIndices = serHeader->getVisibleChunkIndices();
		for (uint32_t i = 0; i < serHeader->m_visibleChunkCount; ++i)
		{
			const uint32_t visibleChunkIndex = serVisibleChunkIndices[i];
			if (!isInvalidIndex(chunkActorIndices[visibleChunkIndex]))
			{
				NVBLASTLL_LOG_ERROR(logFn, "Actor::deserialize: visible chunk already has an actor in family.  Actor not created.");
				header->returnActor(*actor);
				return nullptr;
			}
		}
	}
#endif

	// Visible chunk indices and chunk actor indices
	{
		// Add visible chunks, set chunk subtree ownership
		const uint32_t* serVisibleChunkIndices = serHeader->getVisibleChunkIndices();
		IndexDLink<uint32_t>* visibleChunkIndexLinks = header->getVisibleChunkIndexLinks();
		for (uint32_t i = serHeader->m_visibleChunkCount; i--;)	// Reverse-order, so the resulting linked list is in the original order
		{
			const uint32_t visibleChunkIndex = serVisibleChunkIndices[i];
			NVBLAST_ASSERT(isInvalidIndex(visibleChunkIndexLinks[visibleChunkIndex].m_adj[0]) && isInvalidIndex(visibleChunkIndexLinks[visibleChunkIndex].m_adj[1]));
			IndexDList<uint32_t>().insertListHead(actor->m_firstVisibleChunkIndex, visibleChunkIndexLinks, visibleChunkIndex);
			for (Asset::DepthFirstIt j(*asset, visibleChunkIndex, true); (bool)j; ++j)
			{
				NVBLAST_ASSERT(isInvalidIndex(chunkActorIndices[(uint32_t)j]));
				chunkActorIndices[(uint32_t)j] = actorIndex;
			}
		}
		actor->m_visibleChunkCount = serHeader->m_visibleChunkCount;
	}

	// Graph node indices, leaf chunk count, and and island IDs
	{
		// Add graph nodes
		const uint32_t* serGraphNodeIndices = serHeader->getGraphNodeIndices();
		uint32_t* graphNodeIndexLinks = header->getGraphNodeIndexLinks();
		uint32_t* islandIDs = familyGraph->getIslandIds();
		for (uint32_t i = serHeader->m_graphNodeCount; i--;)	// Reverse-order, so the resulting linked list is in the original order
		{
			const uint32_t graphNodeIndex = serGraphNodeIndices[i];
			NVBLAST_ASSERT(isInvalidIndex(graphNodeIndexLinks[graphNodeIndex]));
			graphNodeIndexLinks[graphNodeIndex] = actor->m_firstGraphNodeIndex;
			actor->m_firstGraphNodeIndex = graphNodeIndex;
			islandIDs[graphNodeIndex] = actorIndex;
		}
		actor->m_graphNodeCount = serHeader->m_graphNodeCount;
		actor->m_leafChunkCount = serHeader->m_leafChunkCount;
	}

	// Lower support chunk healths
	{
		const float* serLowerSupportChunkHealths = serHeader->getLowerSupportChunkHealths();
		float* subsupportHealths = header->getSubsupportChunkHealths();
		const uint32_t subsupportChunkCount = asset->getUpperSupportChunkCount();
		if (actor->m_graphNodeCount > 0)
		{
			uint32_t serLowerSupportChunkCount = 0;
			float* graphNodeHealths = header->getLowerSupportChunkHealths();
			for (Actor::GraphNodeIt i = *actor; (bool)i; ++i)
			{
				const uint32_t graphNodeIndex = (uint32_t)i;
				graphNodeHealths[graphNodeIndex] = serLowerSupportChunkHealths[serLowerSupportChunkCount++];
				const uint32_t supportChunkIndex = graphChunkIndices[graphNodeIndex];
				Asset::DepthFirstIt j(*asset, supportChunkIndex);
				NVBLAST_ASSERT((bool)j);
				++j;	// Skip first (support) chunk, it's already been handled
				for (; (bool)j; ++j)
				{
					subsupportHealths[(uint32_t)j] = serLowerSupportChunkHealths[serLowerSupportChunkCount++];
				}
			}
		}
		else	// Single subsupport chunk
		if (!isInvalidIndex(actor->m_firstVisibleChunkIndex))
		{
			NVBLAST_ASSERT(actor->m_firstVisibleChunkIndex >= subsupportChunkCount);
			subsupportHealths[actor->m_firstVisibleChunkIndex - subsupportChunkCount] = *serLowerSupportChunkHealths;
		}
	}

	// Bond healths
	uint32_t serBondCount = 0;
	{
		const float* serBondHealths = serHeader->getBondHealths();
		float* bondHealths = header->getBondHealths();
		for (Actor::GraphNodeIt i = *actor; (bool)i; ++i)
		{
			const uint32_t graphNodeIndex = (uint32_t)i;
			for (uint32_t adjacentIndex = graphAdjacencyPartition[graphNodeIndex]; adjacentIndex < graphAdjacencyPartition[graphNodeIndex + 1]; ++adjacentIndex)
			{
				const uint32_t adjacentNodeIndex = graphAdjacentNodeIndices[adjacentIndex];
				if (adjacentNodeIndex > graphNodeIndex)	// So as not to double-count
				{
					// Only count if the adjacent node belongs to this actor
					const uint32_t adjacentChunkIndex = graphChunkIndices[adjacentNodeIndex];
					if (chunkActorIndices[adjacentChunkIndex] == actorIndex)
					{
						const uint32_t adjacentBondIndex = graphAdjacentBondIndices[adjacentIndex];
						bondHealths[adjacentBondIndex] = serBondHealths[serBondCount++];
					}
				}
			}
		}
	}

	// Fast routes
	{
		const uint32_t* serFastRoute = serHeader->getFastRoute();
		uint32_t* fastRoute = header->getFamilyGraph()->getFastRoute();
		for (Actor::GraphNodeIt i = *actor; (bool)i; ++i)
		{
			fastRoute[(uint32_t)i] = *serFastRoute++;
		}
	}

	// Hop counts
	{
		const uint32_t* serHopCounts = serHeader->getHopCounts();
		uint32_t* hopCounts = header->getFamilyGraph()->getHopCounts();
		for (Actor::GraphNodeIt i = *actor; (bool)i; ++i)
		{
			hopCounts[(uint32_t)i] = *serHopCounts++;
		}
	}

	// Edge removed array
	if (serBondCount > 0)
	{
		uint32_t serBondIndex = 0;
		const FixedBoolArray* serEdgeRemovedArray = serHeader->getEdgeRemovedArray();
		FixedBoolArray* edgeRemovedArray = familyGraph->getIsEdgeRemoved();
		for (Actor::GraphNodeIt i = *actor; (bool)i; ++i)
		{
			const uint32_t graphNodeIndex = (uint32_t)i;
			for (uint32_t adjacentIndex = graphAdjacencyPartition[graphNodeIndex]; adjacentIndex < graphAdjacencyPartition[graphNodeIndex + 1]; ++adjacentIndex)
			{
				const uint32_t adjacentNodeIndex = graphAdjacentNodeIndices[adjacentIndex];
				if (adjacentNodeIndex > graphNodeIndex)	// So as not to double-count
				{
					// Only count if the adjacent node belongs to this actor
					const uint32_t adjacentChunkIndex = graphChunkIndices[adjacentNodeIndex];
					if (chunkActorIndices[adjacentChunkIndex] == actorIndex)
					{
						if (!serEdgeRemovedArray->test(serBondIndex))
						{
							const uint32_t adjacentBondIndex = graphAdjacentBondIndices[adjacentIndex];
							edgeRemovedArray->reset(adjacentBondIndex);
						}
						++serBondIndex;
					}
				}
			}
		}
	}

	return actor;
}


//////// Actor member methods for serialization ////////

uint32_t Actor::serialize(void* buffer, uint32_t bufferSize, NvBlastLog logFn) const
{
	// Set up pointers and such
	const Asset* asset = getAsset();
	const SupportGraph& graph = asset->m_graph;
	const uint32_t* graphChunkIndices = graph.getChunkIndices();
	const uint32_t* graphAdjacencyPartition = graph.getAdjacencyPartition();
	const uint32_t* graphAdjacentNodeIndices = graph.getAdjacentNodeIndices();
	const uint32_t* graphAdjacentBondIndices = graph.getAdjacentBondIndices();
	const FamilyHeader* header = getFamilyHeader();
	const uint32_t* chunkActorIndices = header->getChunkActorIndices();
	const uint32_t thisActorIndex = getIndex();

	// Make sure there are no dirty nodes
	if (m_graphNodeCount)
	{
		const uint32_t* firstDirtyNodeIndices = header->getFamilyGraph()->getFirstDirtyNodeIndices();
		if (!isInvalidIndex(firstDirtyNodeIndices[thisActorIndex]))
		{
			NVBLASTLL_LOG_ERROR(logFn, "Nv::Blast::Actor::serialize: instance graph has dirty nodes.  Call Nv::Blast::Actor::findIslands before serializing.");
			return 0;
		}
	}

	uint64_t offset = 0;

	// Header
	ActorSerializationHeader* serHeader = reinterpret_cast<ActorSerializationHeader*>(buffer);
	offset = align16(sizeof(ActorSerializationHeader));
	if (offset > bufferSize)
	{
		return 0;	// Buffer size insufficient
	}
	serHeader->m_formatVersion = ActorSerializationFormat::Current;
	serHeader->m_size = 0;	// Will be updated below
	serHeader->m_index = thisActorIndex;
	serHeader->m_visibleChunkCount = m_visibleChunkCount;
	serHeader->m_graphNodeCount = m_graphNodeCount;
	serHeader->m_leafChunkCount = m_leafChunkCount;

	// Visible chunk indices
	{
		serHeader->m_visibleChunkIndicesOffset = (uint32_t)offset;
		offset = align16(offset + m_visibleChunkCount*sizeof(uint32_t));
		if (offset > bufferSize)
		{
			NVBLASTLL_LOG_ERROR(logFn, "Nv::Blast::Actor::Actor::serialize: buffer size exceeded.");
			return 0;	// Buffer size insufficient
		}
		uint32_t* serVisibleChunkIndices = serHeader->getVisibleChunkIndices();
		uint32_t serVisibleChunkCount = 0;
		for (Actor::VisibleChunkIt i = *this; (bool)i; ++i)
		{
			NVBLAST_ASSERT(serVisibleChunkCount < m_visibleChunkCount);
			serVisibleChunkIndices[serVisibleChunkCount++] = (uint32_t)i;
		}
		NVBLAST_ASSERT(serVisibleChunkCount == m_visibleChunkCount);
	}

	// Graph node indices
	{
		serHeader->m_graphNodeIndicesOffset = (uint32_t)offset;
		offset = align16(offset + m_graphNodeCount*sizeof(uint32_t));
		if (offset > bufferSize)
		{
			NVBLASTLL_LOG_ERROR(logFn, "Nv::Blast::Actor::serialize: buffer size exceeded.");
			return 0;	// Buffer size insufficient
		}
		uint32_t* serGraphNodeIndices = serHeader->getGraphNodeIndices();
		uint32_t serGraphNodeCount = 0;
		for (Actor::GraphNodeIt i = *this; (bool)i; ++i)
		{
			NVBLAST_ASSERT(serGraphNodeCount < m_graphNodeCount);
			serGraphNodeIndices[serGraphNodeCount++] = (uint32_t)i;
		}
		NVBLAST_ASSERT(serGraphNodeCount == m_graphNodeCount);
	}

	// Lower support chunk healths
	{
		serHeader->m_lowerSupportChunkHealthsOffset = (uint32_t)offset;
		float* serLowerSupportChunkHealths = serHeader->getLowerSupportChunkHealths();
		const float* subsupportHealths = header->getSubsupportChunkHealths();
		const uint32_t subsupportChunkCount = asset->getUpperSupportChunkCount();
		if (m_graphNodeCount > 0)
		{
			uint32_t serLowerSupportChunkCount = 0;
			const float* graphNodeHealths = header->getLowerSupportChunkHealths();
			for (Actor::GraphNodeIt i = *this; (bool)i; ++i)
			{
				const uint32_t graphNodeIndex = (uint32_t)i;
				serLowerSupportChunkHealths[serLowerSupportChunkCount++] = graphNodeHealths[graphNodeIndex];
				offset += sizeof(float);
				const uint32_t supportChunkIndex = graphChunkIndices[graphNodeIndex];
				Asset::DepthFirstIt j(*asset, supportChunkIndex);
				NVBLAST_ASSERT((bool)j);
				++j;	// Skip first (support) chunk, it's already been handled
				for (; (bool)j; ++j)
				{
					if (offset >= bufferSize)
					{
						NVBLASTLL_LOG_ERROR(logFn, "Nv::Blast::Actor::serialize: buffer size exceeded.");
						return 0;	// Buffer size insufficient
					}
					serLowerSupportChunkHealths[serLowerSupportChunkCount++] = subsupportHealths[(uint32_t)j - subsupportChunkCount];
					offset += sizeof(float);
				}
			}
		}
		else	// Single subsupport chunk
		if (!isInvalidIndex(m_firstVisibleChunkIndex))
		{
			NVBLAST_ASSERT(m_firstVisibleChunkIndex >= subsupportChunkCount);
			if (offset >= bufferSize)
			{
				NVBLASTLL_LOG_ERROR(logFn, "Nv::Blast::Actor::serialize: buffer size exceeded.");
				return 0;	// Buffer size insufficient
			}
			*serLowerSupportChunkHealths = subsupportHealths[m_firstVisibleChunkIndex - subsupportChunkCount];
			offset += sizeof(float);
		}
	}
	offset = align16(offset);

	// Bond healths
	uint32_t serBondCount = 0;
	{
		serHeader->m_bondHealthsOffset = (uint32_t)offset;
		float* serBondHealths = serHeader->getBondHealths();
		const float* bondHealths = header->getBondHealths();
		for (Actor::GraphNodeIt i = *this; (bool)i; ++i)
		{
			const uint32_t graphNodeIndex = (uint32_t)i;
			for (uint32_t adjacentIndex = graphAdjacencyPartition[graphNodeIndex]; adjacentIndex < graphAdjacencyPartition[graphNodeIndex + 1]; ++adjacentIndex)
			{
				const uint32_t adjacentNodeIndex = graphAdjacentNodeIndices[adjacentIndex];
				if (adjacentNodeIndex > graphNodeIndex)	// So as not to double-count
				{
					// Only count if the adjacent node belongs to this actor
					const uint32_t adjacentChunkIndex = graphChunkIndices[adjacentNodeIndex];
					if (chunkActorIndices[adjacentChunkIndex] == thisActorIndex)
					{
						if (offset >= bufferSize)
						{
							NVBLASTLL_LOG_ERROR(logFn, "Nv::Blast::Actor::serialize: buffer size exceeded.");
							return 0;	// Buffer size insufficient
						}
						const uint32_t adjacentBondIndex = graphAdjacentBondIndices[adjacentIndex];
						serBondHealths[serBondCount++] = bondHealths[adjacentBondIndex];
						offset += sizeof(float);
					}
				}
			}
		}
	}
	offset = align16(offset);

	// Fast routes
	{
		serHeader->m_fastRouteOffset = (uint32_t)offset;
		offset = align16(offset + m_graphNodeCount*sizeof(uint32_t));
		if (offset > bufferSize)
		{
			NVBLASTLL_LOG_ERROR(logFn, "Nv::Blast::Actor::serialize: buffer size exceeded.");
			return 0;	// Buffer size insufficient
		}
		uint32_t* serFastRoute = serHeader->getFastRoute();
		const uint32_t* fastRoute = header->getFamilyGraph()->getFastRoute();
		for (Actor::GraphNodeIt i = *this; (bool)i; ++i)
		{
			*serFastRoute++ = fastRoute[(uint32_t)i];
		}
	}

	// Hop counts
	{
		serHeader->m_hopCountsOffset = (uint32_t)offset;
		offset = align16(offset + m_graphNodeCount*sizeof(uint32_t));
		if (offset > bufferSize)
		{
			NVBLASTLL_LOG_ERROR(logFn, "Nv::Blast::Actor::serialize: buffer size exceeded.");
			return 0;	// Buffer size insufficient
		}
		uint32_t* serHopCounts = serHeader->getHopCounts();
		const uint32_t* hopCounts = header->getFamilyGraph()->getHopCounts();
		for (Actor::GraphNodeIt i = *this; (bool)i; ++i)
		{
			*serHopCounts++ = hopCounts[(uint32_t)i];
		}
	}

	// Edge removed array
	if (serBondCount > 0)
	{
		serHeader->m_edgeRemovedArrayOffset = (uint32_t)offset;
		offset = align16(offset + FixedBoolArray::requiredMemorySize(serBondCount));
		if (offset > bufferSize)
		{
			NVBLASTLL_LOG_ERROR(logFn, "Nv::Blast::Actor::serialize: buffer size exceeded.");
			return 0;	// Buffer size insufficient
		}
		uint32_t serBondIndex = 0;
		FixedBoolArray* serEdgeRemovedArray = serHeader->getEdgeRemovedArray();
		new (serEdgeRemovedArray)FixedBoolArray(serBondCount);
		serEdgeRemovedArray->fill();	// Reset bits as we find bonds
		const FixedBoolArray* edgeRemovedArray = header->getFamilyGraph()->getIsEdgeRemoved();
		for (Actor::GraphNodeIt i = *this; (bool)i; ++i)
		{
			const uint32_t graphNodeIndex = (uint32_t)i;
			for (uint32_t adjacentIndex = graphAdjacencyPartition[graphNodeIndex]; adjacentIndex < graphAdjacencyPartition[graphNodeIndex + 1]; ++adjacentIndex)
			{
				const uint32_t adjacentNodeIndex = graphAdjacentNodeIndices[adjacentIndex];
				if (adjacentNodeIndex > graphNodeIndex)	// So as not to double-count
				{
					// Only count if the adjacent node belongs to this actor
					const uint32_t adjacentChunkIndex = graphChunkIndices[adjacentNodeIndex];
					if (chunkActorIndices[adjacentChunkIndex] == thisActorIndex)
					{
						const uint32_t adjacentBondIndex = graphAdjacentBondIndices[adjacentIndex];
						if (!edgeRemovedArray->test(adjacentBondIndex))
						{
							serEdgeRemovedArray->reset(serBondIndex);
						}
						++serBondIndex;
					}
				}
			}
		}
	}

	// Finally record size
	serHeader->m_size = static_cast<uint32_t>(offset);

	return serHeader->m_size;
}


uint32_t Actor::serializationRequiredStorage(NvBlastLog logFn) const
{
	const Asset* asset = getAsset();
	const SupportGraph& graph = asset->m_graph;
	const uint32_t* graphChunkIndices = graph.getChunkIndices();
	const uint32_t* graphAdjacencyPartition = graph.getAdjacencyPartition();
	const uint32_t* graphAdjacentNodeIndices = graph.getAdjacentNodeIndices();
	const uint32_t* graphNodeIndexLinks = getFamilyHeader()->getGraphNodeIndexLinks();
	const uint32_t* chunkActorIndices = getFamilyHeader()->getChunkActorIndices();
	const uint32_t thisActorIndex = getIndex();

	// Lower-support chunk count and bond counts for this actor need to be calculated.  Iterate over all support chunks to count these.
	uint32_t lowerSupportChunkCount = 0;
	uint32_t bondCount = 0;
	if (m_graphNodeCount > 0)
	{
		for (uint32_t graphNodeIndex = m_firstGraphNodeIndex; !isInvalidIndex(graphNodeIndex); graphNodeIndex = graphNodeIndexLinks[graphNodeIndex])
		{
			// Update bond count
			const uint32_t supportChunkIndex = graphChunkIndices[graphNodeIndex];
			for (uint32_t adjacentIndex = graphAdjacencyPartition[graphNodeIndex]; adjacentIndex < graphAdjacencyPartition[graphNodeIndex + 1]; ++adjacentIndex)
			{
				const uint32_t adjacentNodeIndex = graphAdjacentNodeIndices[adjacentIndex];
				if (adjacentNodeIndex > graphNodeIndex)	// So as not to double-count
				{
					// Only count if the adjacent node belongs to this actor
					const uint32_t adjacentChunkIndex = graphChunkIndices[adjacentNodeIndex];
					if (chunkActorIndices[adjacentChunkIndex] == thisActorIndex)
					{
						++bondCount;
					}
				}
			}

			// Update lower-support chunk count
			for (Asset::DepthFirstIt i(*asset, supportChunkIndex); (bool)i; ++i)
			{
				++lowerSupportChunkCount;
			}
		}
	}
	else	// Subsupport chunk
	{
		++lowerSupportChunkCount;
	}

	const uint64_t dataSize = getActorSerializationSize(m_visibleChunkCount, lowerSupportChunkCount, m_graphNodeCount, bondCount);

	if (dataSize > UINT32_MAX)
	{
		NVBLASTLL_LOG_WARNING(logFn, "Nv::Blast::Actor::serializationRequiredStorage: Serialization block size exceeds 4GB.  Returning 0.\n");
		return 0;
	}

	return static_cast<uint32_t>(dataSize);
}

} // namespace Blast
} // namespace Nv


// API implementation

extern "C"
{

uint32_t NvBlastActorGetSerializationSize(const NvBlastActor* actor, NvBlastLog logFn)
{
	NVBLASTLL_CHECK(actor != nullptr, logFn, "NvBlastActorGetSerializationSize: NULL actor pointer input.", return 0);

	const Nv::Blast::Actor& a = *static_cast<const Nv::Blast::Actor*>(actor);

	if (!a.isActive())
	{
		NVBLASTLL_LOG_ERROR(logFn, "NvBlastActorGetSerializationSize: inactive actor pointer input.");
		return 0;
	}

	return a.serializationRequiredStorage(logFn);
}


uint32_t NvBlastActorSerialize(void* buffer, uint32_t bufferSize, const NvBlastActor* actor, NvBlastLog logFn)
{
	NVBLASTLL_CHECK(buffer != nullptr, logFn, "NvBlastActorSerialize: NULL buffer pointer input.", return 0);
	NVBLASTLL_CHECK(actor != nullptr, logFn, "NvBlastActorSerialize: NULL actor pointer input.", return 0);

	const Nv::Blast::Actor& a = *static_cast<const Nv::Blast::Actor*>(actor);

	if (!a.isActive())
	{
		NVBLASTLL_LOG_ERROR(logFn, "NvBlastActorSerialize: inactive actor pointer input.");
		return 0;
	}

	return a.serialize(buffer, bufferSize, logFn);
}


NvBlastActor* NvBlastFamilyDeserializeActor(NvBlastFamily* family, const void* buffer, NvBlastLog logFn)
{
	NVBLASTLL_CHECK(family != nullptr, logFn, "NvBlastFamilyDeserializeActor: NULL family input.  No actor deserialized.", return nullptr);
	NVBLASTLL_CHECK(buffer != nullptr, logFn, "NvBlastFamilyDeserializeActor: NULL buffer pointer input.  No actor deserialized.", return nullptr);

	return Nv::Blast::Actor::deserialize(family, buffer, logFn);
}

} // extern "C"
