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
// Copyright (c) 2020 NVIDIA Corporation. All rights reserved.


#include "AssetDTO.h"
#include "NvBlastGlobals.h"
#include "NvBlastIDDTO.h"
#include "NvBlastChunkDTO.h"
#include "NvBlastBondDTO.h"
#include "NvBlastAsset.h"


namespace Nv
{
namespace Blast
{

bool AssetDTO::serialize(Nv::Blast::Serialization::Asset::Builder builder, const Nv::Blast::Asset * poco)
{
	NvBlastIDDTO::serialize(builder.initID(), &poco->m_ID);

	builder.setLeafChunkCount(poco->m_leafChunkCount);

	builder.setFirstSubsupportChunkIndex(poco->m_firstSubsupportChunkIndex);

	capnp::List<Nv::Blast::Serialization::NvBlastChunk>::Builder chunks = builder.initChunks(poco->m_chunkCount);

	builder.setChunkCount(poco->m_chunkCount);

	NVBLAST_ASSERT_WITH_MESSAGE(builder.getChunkCount() == poco->m_chunkCount, "WTF");

	for (uint32_t i = 0; i < poco->m_chunkCount; i++)
	{
		NvBlastChunk& chunk = poco->getChunks()[i];

		NvBlastChunkDTO::serialize(chunks[i], &chunk);
	}

	NVBLAST_ASSERT_WITH_MESSAGE(builder.getChunkCount() == poco->m_chunkCount, "WTF");

	capnp::List<Nv::Blast::Serialization::NvBlastBond>::Builder bonds = builder.initBonds(poco->m_bondCount);

	builder.setBondCount(poco->m_bondCount);

	for (uint32_t i = 0; i < poco->m_bondCount; i++)
	{
		NvBlastBond& bond = poco->getBonds()[i];

		NvBlastBondDTO::serialize(bonds[i], &bond);
	}

	kj::ArrayPtr<uint32_t> stlcArray(poco->getSubtreeLeafChunkCounts(), poco->m_chunkCount);
	builder.initSubtreeLeafChunkCounts(poco->m_chunkCount);
	builder.setSubtreeLeafChunkCounts(stlcArray);

	kj::ArrayPtr<uint32_t> ctgnArray(poco->getChunkToGraphNodeMap(), poco->m_chunkCount);
	builder.setChunkToGraphNodeMap(ctgnArray);

	Nv::Blast::Serialization::NvBlastSupportGraph::Builder graphBulder = builder.initGraph();

	graphBulder.setNodeCount(poco->m_graph.m_nodeCount);

	uint32_t* ciPtr = poco->m_graph.getChunkIndices();

	kj::ArrayPtr<const uint32_t> ciArray(ciPtr, poco->m_graph.m_nodeCount);
	graphBulder.setChunkIndices(ciArray);

	kj::ArrayPtr<const uint32_t> adjPart(poco->m_graph.getAdjacencyPartition(), poco->m_graph.m_nodeCount + 1);
	graphBulder.setAdjacencyPartition(adjPart);

	NVBLAST_ASSERT(graphBulder.getAdjacencyPartition().size() == poco->m_graph.m_nodeCount + 1);

	kj::ArrayPtr<const uint32_t> nodeIndices(poco->m_graph.getAdjacentNodeIndices(), poco->m_bondCount * 2);
	graphBulder.setAdjacentNodeIndices(nodeIndices);

	NVBLAST_ASSERT(graphBulder.getAdjacentNodeIndices().size() == poco->m_bondCount * 2);

	kj::ArrayPtr<const uint32_t> bondIndices(poco->m_graph.getAdjacentBondIndices(), poco->m_bondCount * 2);
	graphBulder.setAdjacentBondIndices(bondIndices);

	return true;
}


Nv::Blast::Asset* AssetDTO::deserialize(Nv::Blast::Serialization::Asset::Reader reader)
{
	NvBlastID EmptyId;
	memset(EmptyId.data, 0, sizeof(NvBlastID));

	void* mem = NVBLAST_ALLOC(reader.totalSize().wordCount * sizeof(uint64_t));

	auto asset = Nv::Blast::initializeAsset(mem, EmptyId, reader.getChunkCount(), reader.getGraph().getNodeCount(), reader.getLeafChunkCount(), reader.getFirstSubsupportChunkIndex(), reader.getBondCount(), logLL);

	bool result = deserializeInto(reader, asset);

	return result ? asset : nullptr;
}


bool AssetDTO::deserializeInto(Nv::Blast::Serialization::Asset::Reader reader, Nv::Blast::Asset * poco)
{
	NvBlastIDDTO::deserializeInto(reader.getID(), &poco->m_ID);

	NvBlastBond* bonds = poco->getBonds();

	uint32_t bondCount = reader.getBondCount();
	auto readerBonds = reader.getBonds();
	for (uint32_t i = 0; i < bondCount; i++)
	{
		auto bondReader = readerBonds[i];

		NvBlastBondDTO::deserializeInto(bondReader, &bonds[i]);
	}

	NvBlastChunk* chunks = poco->getChunks();

	uint32_t chunkCount = reader.getChunkCount();
	auto readerChunks = reader.getChunks();
	for (uint32_t i = 0; i < chunkCount; i++)
	{
		auto chunkReader = readerChunks[i];

		NvBlastChunkDTO::deserializeInto(chunkReader, &chunks[i]);
	}

	poco->m_graph.m_nodeCount = reader.getGraph().getNodeCount();

	NVBLAST_ASSERT(reader.getSubtreeLeafChunkCounts().size() == poco->m_chunkCount);
	auto readerSubtreeLeafChunkCounts = reader.getSubtreeLeafChunkCounts();
	for (uint32_t i = 0; i < poco->m_chunkCount; i++)
	{
		poco->getSubtreeLeafChunkCounts()[i] = readerSubtreeLeafChunkCounts[i];
	}

	auto readerChunkToGraphNodeMap = reader.getChunkToGraphNodeMap();
	for (uint32_t i = 0; i < chunkCount; i++)
	{
		poco->getChunkToGraphNodeMap()[i] = readerChunkToGraphNodeMap[i];
	}

	uint32_t* ciPtr = poco->m_graph.getChunkIndices();

	NVBLAST_ASSERT(reader.getGraph().getChunkIndices().size() == poco->m_graph.m_nodeCount);
	auto readerGraphChunkIndices = reader.getGraph().getChunkIndices();
	for (uint32_t i = 0; i < poco->m_graph.m_nodeCount; i++)
	{
		ciPtr[i] = readerGraphChunkIndices[i];
	}

	uint32_t* adjPartition = poco->m_graph.getAdjacencyPartition();
	const uint32_t graphAdjacencyPartitionSize = reader.getGraph().getAdjacencyPartition().size();
	auto readerGraphAdjacencyPartition = reader.getGraph().getAdjacencyPartition();
	for (uint32_t i = 0; i < graphAdjacencyPartitionSize; ++i)
	{
		adjPartition[i] = readerGraphAdjacencyPartition[i];
	}

	uint32_t* adjNodes = poco->m_graph.getAdjacentNodeIndices();
	const uint32_t graphAdjacentNodeIndicesSize = reader.getGraph().getAdjacentNodeIndices().size();
	auto readerGraphAdjacentNodeIndices = reader.getGraph().getAdjacentNodeIndices();
	for (uint32_t i = 0; i < graphAdjacentNodeIndicesSize; ++i)
	{
		adjNodes[i] = readerGraphAdjacentNodeIndices[i];
	}

	uint32_t* adjBonds = poco->m_graph.getAdjacentBondIndices();
	const uint32_t graphAdjacentBondIndicesSize = reader.getGraph().getAdjacentBondIndices().size();
	auto readerGraphAdjacentBondIndices = reader.getGraph().getAdjacentBondIndices();
	for (uint32_t i = 0; i < graphAdjacentBondIndicesSize; ++i)
	{
		adjBonds[i] = readerGraphAdjacentBondIndices[i];
	}

	return true;
}

}	// namespace Blast
}	// namespace Nv
