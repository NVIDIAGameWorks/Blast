/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/


#include "NvBlastFamily.h"
#include "NvBlastFamilyGraph.h"
#include "NvBlastIndexFns.h"

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
	const Nv::Blast::SupportGraph& graph = asset->m_graph;

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
#if NVBLAST_CHECK_PARAMS
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
	NVBLAST_CHECK(mem != nullptr, logFn, "createFamily: NULL mem pointer input.", return nullptr);
	NVBLAST_CHECK(asset != nullptr, logFn, "createFamily: NULL asset pointer input.", return nullptr);

	NVBLAST_CHECK((reinterpret_cast<uintptr_t>(mem) & 0xF) == 0, logFn, "createFamily: mem pointer not 16-byte aligned.", return nullptr);

	const Asset& solverAsset = *static_cast<const Asset*>(asset);

	if (solverAsset.m_chunkCount == 0)
	{
		NVBLAST_LOG_ERROR(logFn, "createFamily: Asset has no chunks.  Family not created.\n");
		return nullptr;
	}

	const Nv::Blast::SupportGraph& graph = solverAsset.m_graph;

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
		NVBLAST_LOG_ERROR(logFn, "Nv::Blast::Actor::instanceAllocate: Instance data block size will exceed 4GB.  Instance not created.\n");
		return nullptr;
	}

	// Allocate family
	NvBlastFamily* family = (NvBlastFamily*)mem;

	// Fill in family header
	FamilyHeader* header = (FamilyHeader*)family;
	header->dataType = NvBlastDataBlock::FamilyDataBlock;
	header->formatVersion = NvBlastFamilyDataFormat::Current;
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
	NVBLAST_CHECK(family != nullptr, logFn, "NvBlastFamilyGetFormatVersion: NULL family pointer input.", return UINT32_MAX);
	return reinterpret_cast<const Nv::Blast::FamilyHeader*>(family)->formatVersion;
}


void NvBlastFamilySetAsset(NvBlastFamily* family, const NvBlastAsset* asset, NvBlastLog logFn)
{
	NVBLAST_CHECK(family != nullptr, logFn, "NvBlastFamilySetAsset: NULL family pointer input.", return);
	NVBLAST_CHECK(asset != nullptr, logFn, "NvBlastFamilySetAsset: NULL asset pointer input.", return);

	Nv::Blast::FamilyHeader* header = reinterpret_cast<Nv::Blast::FamilyHeader*>(family);
	const Nv::Blast::Asset* solverAsset = reinterpret_cast<const Nv::Blast::Asset*>(asset);

	if (memcmp(&header->m_assetID, &solverAsset->m_ID, sizeof(NvBlastID)))
	{
		NVBLAST_LOG_ERROR(logFn, "NvBlastFamilySetAsset: wrong asset.  Passed asset ID doesn't match family asset ID.");
		return;
	}

	header->m_asset = solverAsset;
}


uint32_t NvBlastFamilyGetSize(const NvBlastFamily* family, NvBlastLog logFn)
{
	NVBLAST_CHECK(family != nullptr, logFn, "NvBlastFamilyGetSize: NULL family pointer input.", return 0);
	return reinterpret_cast<const Nv::Blast::FamilyHeader*>(family)->size;
}


NvBlastID NvBlastFamilyGetAssetID(const NvBlastFamily* family, NvBlastLog logFn)
{
	NVBLAST_CHECK(family != nullptr, logFn, "NvBlastFamilyGetAssetID: NULL family pointer input.", return NvBlastID());
	return reinterpret_cast<const Nv::Blast::FamilyHeader*>(family)->m_assetID;
}


uint32_t NvBlastFamilyGetActorCount(const NvBlastFamily* family, NvBlastLog logFn)
{
	NVBLAST_CHECK(family != nullptr, logFn, "NvBlastFamilyGetActorCount: NULL family pointer input.", return 0);

	const Nv::Blast::FamilyHeader* header = reinterpret_cast<const Nv::Blast::FamilyHeader*>(family);

	if (header->formatVersion != NvBlastFamilyDataFormat::Current)
	{
		NVBLAST_LOG_ERROR(logFn, "NvBlastFamilyGetActorCount: wrong family format.  Family must be converted to current version.");
		return 0;
	}

	return header->m_actorCount;
}


uint32_t NvBlastFamilyGetActors(NvBlastActor** actors, uint32_t actorsSize, const NvBlastFamily* family, NvBlastLog logFn)
{
	NVBLAST_CHECK(actors != nullptr, logFn, "NvBlastFamilyGetActors: NULL actors pointer input.", return 0);
	NVBLAST_CHECK(family != nullptr, logFn, "NvBlastFamilyGetActors: NULL family pointer input.", return 0);

	const Nv::Blast::FamilyHeader* header = reinterpret_cast<const Nv::Blast::FamilyHeader*>(family);

	if (header->formatVersion != NvBlastFamilyDataFormat::Current)
	{
		NVBLAST_LOG_ERROR(logFn, "NvBlastFamilyGetActors: wrong family format.  Family must be converted to current version.");
		return 0;
	}

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
	NVBLAST_CHECK(family != nullptr, logFn, "NvBlastFamilyGetChunkActor: NULL family pointer input.", return nullptr);

	const Nv::Blast::FamilyHeader* header = reinterpret_cast<const Nv::Blast::FamilyHeader*>(family);

	NVBLAST_CHECK(header->m_asset != nullptr, logFn, "NvBlastFamilyGetChunkActor: NvBlastFamily has null asset set.", return nullptr);

	const Nv::Blast::Asset& solverAsset = *static_cast<const Nv::Blast::Asset*>(header->m_asset);
	NVBLAST_CHECK(chunkIndex < solverAsset.m_chunkCount, logFn, "NvBlastFamilyGetChunkActor: bad value of chunkIndex for the given family's asset.", return nullptr);

	// get actorIndex from chunkIndex
	uint32_t actorIndex;
	if (chunkIndex < solverAsset.getUpperSupportChunkCount())
	{
		actorIndex = header->getChunkActorIndices()[chunkIndex];
	}
	else
	{
		actorIndex = chunkIndex - (solverAsset.getUpperSupportChunkCount() - solverAsset.m_graph.m_nodeCount);
	}

	// get actor from actorIndex
	if (!Nv::Blast::isInvalidIndex(actorIndex))
	{
		NVBLAST_ASSERT(actorIndex < header->getActorBufferSize());
		Nv::Blast::Actor* actor = &header->getActors()[actorIndex];
		if (actor->isActive())
		{
			return actor;
		}
	}
	return nullptr;
}


uint32_t NvBlastFamilyGetMaxActorCount(const NvBlastFamily* family, NvBlastLog logFn)
{
	NVBLAST_CHECK(family != nullptr, logFn, "NvBlastFamilyGetMaxActorCount: NULL family pointer input.", return 0);
	const Nv::Blast::FamilyHeader* header = reinterpret_cast<const Nv::Blast::FamilyHeader*>(family);
	return header->getActorBufferSize();
}

} // extern "C"
