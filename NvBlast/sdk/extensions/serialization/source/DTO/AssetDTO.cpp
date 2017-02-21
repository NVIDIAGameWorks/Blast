/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "AssetDTO.h"
#include "NvBlastIDDTO.h"
#include "NvBlastChunkDTO.h"
#include "NvBlastBondDTO.h"
#include "NvBlastAsset.h"
#include "NvBlastExtSerializationLLImpl.h"
#include "NvBlastExtGlobals.h"

#if !defined(BLAST_LL_ALLOC)
#include "NvBlastExtAllocator.h"
#endif

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
			NvBlastID EmptyId = {};

			NvBlastExtAlloc allocFn = gAlloc;
			NvBlastLog logFn = gLog;

#if !defined(BLAST_LL_ALLOC)
			allocFn = ExtAllocator::alignedAlloc16;
			logFn = NvBlastTkFrameworkGet()->getLogFn();
#endif

			void* mem = allocFn(reader.totalSize().wordCount * sizeof(uint64_t));

			auto asset = Nv::Blast::initializeAsset(mem, EmptyId, reader.getChunkCount(), reader.getGraph().getNodeCount(), reader.getLeafChunkCount(), reader.getFirstSubsupportChunkIndex(), reader.getBondCount(),
				logFn);
		
			bool result = deserializeInto(reader, asset);
		
			return result ? asset : nullptr;
		}
		
		bool AssetDTO::deserializeInto(Nv::Blast::Serialization::Asset::Reader reader, Nv::Blast::Asset * poco)
		{
			NvBlastIDDTO::deserializeInto(reader.getID(), &poco->m_ID);
		
			NvBlastBond* bonds = poco->getBonds();
		
			uint32_t bondCount = reader.getBondCount();
			for (uint32_t i = 0; i < bondCount; i++)
			{
				auto bondReader = reader.getBonds()[i];
		
				NvBlastBondDTO::deserializeInto(bondReader, &bonds[i]);
			}
		
			NvBlastChunk* chunks = poco->getChunks();
		
			uint32_t chunkCount = reader.getChunkCount();
			for (uint32_t i = 0; i < chunkCount; i++)
			{
				auto chunkReader = reader.getChunks()[i];
		
				NvBlastChunkDTO::deserializeInto(chunkReader, &chunks[i]);
			}
		
			poco->m_graph.m_nodeCount = reader.getGraph().getNodeCount();
		
			NVBLAST_ASSERT(reader.getSubtreeLeafChunkCounts().size() == poco->m_chunkCount);
			for (uint32_t i = 0; i < poco->m_chunkCount; i++)
			{
				poco->getSubtreeLeafChunkCounts()[i] = reader.getSubtreeLeafChunkCounts()[i];
			}
		
			for (uint32_t i = 0; i < chunkCount; i++)
			{
				poco->getChunkToGraphNodeMap()[i] = reader.getChunkToGraphNodeMap()[i];
			}
		
			uint32_t* ciPtr = poco->m_graph.getChunkIndices();
		
			NVBLAST_ASSERT(reader.getGraph().getChunkIndices().size() == poco->m_graph.m_nodeCount);
			for (uint32_t i = 0; i < poco->m_graph.m_nodeCount; i++)
			{
				ciPtr[i] = reader.getGraph().getChunkIndices()[i];
			}
		
			uint32_t* adjPartition = poco->m_graph.getAdjacencyPartition();
			uint32_t idx = 0;
		
			for (uint32_t adjPartIndex : reader.getGraph().getAdjacencyPartition())
			{
				adjPartition[idx++] = adjPartIndex;
			}
		
			uint32_t* adjNodes = poco->m_graph.getAdjacentNodeIndices();
			idx = 0;
		
			for (uint32_t adjNodeIndex : reader.getGraph().getAdjacentNodeIndices())
			{
				adjNodes[idx++] = adjNodeIndex;
			}
		
			uint32_t* adjBonds = poco->m_graph.getAdjacentBondIndices();
			idx = 0;
		
			for (uint32_t adjBondIndex : reader.getGraph().getAdjacentBondIndices())
			{
				adjBonds[idx++] = adjBondIndex;
			}
		
			return true;
		}
	}
}
