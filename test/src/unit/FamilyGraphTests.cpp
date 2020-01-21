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


#include "BlastBaseTest.h"

#include "NvBlastSupportGraph.h"
#include "NvBlastFamilyGraph.h"
#include "NvBlastAssert.h"
#include "NvBlastIndexFns.h"

#include <stdlib.h>
#include <ostream>
#include <stdint.h>
#include <map>
#include <algorithm>


// ====================================================================================================================
//													  HELPERS
// ====================================================================================================================

::testing::AssertionResult VectorMatch(const std::vector<uint32_t>& actual, const uint32_t* expected, uint32_t size)
{
	for (size_t i(0); i < size; ++i)
	{
		if (expected[i] != actual[i])
		{
			testing::Message msg;
			msg << "array[" << i
				<< "] (" << actual[i] << ") != expected[" << i
				<< "] (" << expected[i] << ")";
			return (::testing::AssertionFailure(msg));;
		}
	}

	return ::testing::AssertionSuccess();
}

#define VECTOR_MATCH(actual, ...) \
{ \
	const uint32_t arr[] = { __VA_ARGS__ }; \
	const uint32_t size = (sizeof(arr) / sizeof(arr[0])); \
	EXPECT_EQ(size, actual.size()); \
	EXPECT_TRUE(VectorMatch(actual, arr, size)); \
} 


// ====================================================================================================================
//													   TEST CLASS
// ====================================================================================================================

using namespace Nv::Blast;

template<int FailLevel, int Verbosity>
class FamilyGraphTest : public BlastBaseTest < FailLevel, Verbosity >
{
public:
	FamilyGraphTest()
	{
	}

protected:
	FamilyGraph* buildFamilyGraph(uint32_t chunkCount, const uint32_t* adjacentChunkPartition, const uint32_t* adjacentChunkIndices)
	{
		NVBLAST_ASSERT(m_memoryBlock.size() == 0); // can't build twice per test

		// Fill SupportGraph with data:
		NvBlastCreateOffsetStart(sizeof(SupportGraph));
		const size_t NvBlastCreateOffsetAlign16(chunkIndicesOffset, chunkCount*sizeof(uint32_t));
		const size_t NvBlastCreateOffsetAlign16(adjacencyPartitionOffset, (chunkCount + 1)*sizeof(uint32_t));
		const size_t NvBlastCreateOffsetAlign16(adjacentNodeIndicesOffset, adjacentChunkPartition[chunkCount] * sizeof(uint32_t));
		const size_t NvBlastCreateOffsetAlign16(adjacentBondIndicesOffset, adjacentChunkPartition[chunkCount] * sizeof(uint32_t));
		const size_t graphDataSize = NvBlastCreateOffsetEndAlign16();
		m_graphMemory.resize(graphDataSize);
		m_graph = reinterpret_cast<SupportGraph*>(m_graphMemory.data());

		m_graph->m_nodeCount = chunkCount;
		m_graph->m_chunkIndicesOffset = static_cast<uint32_t>(chunkIndicesOffset);
		m_graph->m_adjacencyPartitionOffset = static_cast<uint32_t>(adjacencyPartitionOffset);
		m_graph->m_adjacentNodeIndicesOffset = static_cast<uint32_t>(adjacentNodeIndicesOffset);
		m_graph->m_adjacentBondIndicesOffset = static_cast<uint32_t>(adjacentBondIndicesOffset);

		memcpy(m_graph->getAdjacencyPartition(), adjacentChunkPartition, (chunkCount + 1) * sizeof(uint32_t));
		memcpy(m_graph->getAdjacentNodeIndices(), adjacentChunkIndices, adjacentChunkPartition[chunkCount] * sizeof(uint32_t));

		// fill bondIndices by incrementing bondIndex and putting same bondIndex in mirror bond index for (n0, n1) == (n1, n0)
		memset(m_graph->getAdjacentBondIndices(), (uint32_t)-1, adjacentChunkPartition[chunkCount] * sizeof(uint32_t));
		uint32_t bondIndex = 0;
		for (uint32_t chunk0 = 0; chunk0 < m_graph->m_nodeCount; chunk0++)
		{
			for (uint32_t i = m_graph->getAdjacencyPartition()[chunk0]; i < m_graph->getAdjacencyPartition()[chunk0 + 1]; i++)
			{
				if (m_graph->getAdjacentBondIndices()[i] == (uint32_t)-1)
				{
					m_graph->getAdjacentBondIndices()[i] = bondIndex;

					uint32_t chunk1 = m_graph->getAdjacentNodeIndices()[i];
					for (uint32_t j = m_graph->getAdjacencyPartition()[chunk1]; j < m_graph->getAdjacencyPartition()[chunk1 + 1]; j++)
					{
						if (m_graph->getAdjacentNodeIndices()[j] == chunk0)
						{
							m_graph->getAdjacentBondIndices()[j] = bondIndex;
						}
					}
					bondIndex++;
				}
			}
		}

		// reserve memory for family graph and asset pointer
		uint32_t familyGraphMemorySize = (uint32_t)FamilyGraph::requiredMemorySize(m_graph->m_nodeCount, bondIndex);
		m_memoryBlock.resize(familyGraphMemorySize);
		// placement new family graph
		FamilyGraph* familyGraph = new(m_memoryBlock.data()) FamilyGraph(m_graph);

		return familyGraph;
	}

	struct IslandInfo
	{
		std::vector<NodeIndex> nodes;
	};

	/**
	Function to gather islands info for tests and debug purposes
	Returned islands sorted by nodes counts. Island nodes also sorted by NodeIndex.
	*/
	void getIslandsInfo(const FamilyGraph& graph, std::vector<IslandInfo>& info)
	{
		IslandId* islandIds = graph.getIslandIds();

		std::map<IslandId, IslandInfo> islandMap;

		for (NodeIndex n = 0; n < m_graph->m_nodeCount; n++)
		{
			EXPECT_TRUE(islandIds[n] != invalidIndex<uint32_t>());
			IslandId islandId = islandIds[n];
			if (islandMap.find(islandId) == islandMap.end())
			{
				IslandInfo info;
				info.nodes.push_back(n);
				islandMap[islandId] = info;
			}
			else
			{
				islandMap[islandId].nodes.push_back(n);
			}
		}

		for (auto it = islandMap.begin(); it != islandMap.end(); ++it)
		{
			std::sort(it->second.nodes.begin(), it->second.nodes.end());
			info.push_back(it->second);
		}

		// sort islands by size ascending
		std::sort(info.begin(), info.end(), [](const IslandInfo& i0, const IslandInfo& i1) -> bool
		{
			size_t s0 = i0.nodes.size();
			size_t s1 = i1.nodes.size();
			if (s0 == s1 && s0 > 0)
			{
				s0 = i0.nodes[0];
				s1 = i1.nodes[0];
			}
			return s0 < s1;
		});
	}

	static const uint32_t DEFAULT_ACTOR_INDEX = 0;

	SupportGraph* m_graph;
	std::vector<char> m_graphMemory;
	std::vector<char> m_memoryBlock;
};

typedef FamilyGraphTest<NvBlastMessage::Error, 1> FamilyGraphTestAllowWarnings;
typedef FamilyGraphTest<NvBlastMessage::Warning, 1> FamilyGraphTestStrict;


// ====================================================================================================================
//													GRAPH DATA
// ====================================================================================================================

//		Graph 0:
//
//     0 -- 1 -- 2 -- 3
//     |    |    |    |
//     |    |    |    |
//	   4 -- 5    6 -- 7
//
const uint32_t chunkCount0 = 8;
const uint32_t adjacentChunkPartition0[] = { 0, 2, 5, 8, 10, 12, 14, 16, 18 };
const uint32_t adjacentChunkIndices0[] = { /*0*/ 1, 4, /*1*/ 0, 2, 5, /*2*/ 1, 3, 6, /*3*/ 2, 7, /*4*/ 0, 5, /*5*/ 1, 4, /*6*/ 2, 7, /*7*/ 3, 6 };


//		Graph 1:
//
//     0 -- 1 -- 2 -- 3
//     |    |    |    |
//     4 -- 5 -- 6 -- 7
//     |    |    |    |
//	   8 -- 9 -- 10-- 11
//
const uint32_t chunkCount1 = 12;
const uint32_t adjacentChunkPartition1[] = { 0, 2, 5, 8, 10, 13, 17, 21, 24, 26, 29, 32, 34 };
const uint32_t adjacentChunkIndices1[] = { /*0*/ 1, 4, /*1*/ 0, 2, 5, /*2*/ 1, 3, 6, /*3*/ 2, 7, /*4*/ 0, 5, 8, /*5*/ 1, 4, 6, 9, /*6*/ 2, 5, 7, 10,
										   /*7*/ 3, 6, 11, /*8*/ 4, 9, /*9*/ 5, 8, 10, /*10*/ 6, 9, 11, /*11*/ 7, 10 };


// ====================================================================================================================
//													   TESTS
// ====================================================================================================================

TEST_F(FamilyGraphTestStrict, Graph0FindIslands0)
{
	FamilyGraph* graph = buildFamilyGraph(chunkCount0, adjacentChunkPartition0, adjacentChunkIndices0);
	graph->initialize(DEFAULT_ACTOR_INDEX, m_graph);

	std::vector<char> scratch;
	scratch.resize((size_t)FamilyGraph::findIslandsRequiredScratch(chunkCount0));

	EXPECT_EQ(9, graph->getEdgesCount(m_graph));
	graph->notifyEdgeRemoved(DEFAULT_ACTOR_INDEX, 0, 4, m_graph);
	EXPECT_EQ(8, graph->getEdgesCount(m_graph));
	EXPECT_EQ(1, graph->findIslands(DEFAULT_ACTOR_INDEX, scratch.data(), m_graph));

	graph->notifyEdgeRemoved(DEFAULT_ACTOR_INDEX, 1, 2, m_graph);
	EXPECT_EQ(1, graph->findIslands(DEFAULT_ACTOR_INDEX, scratch.data(), m_graph));

	std::vector<IslandInfo> info;
	getIslandsInfo(*graph, info);
	EXPECT_EQ(2, info.size());
	VECTOR_MATCH(info[0].nodes, 0, 1, 4, 5);
	VECTOR_MATCH(info[1].nodes, 2, 3, 6, 7);
}

TEST_F(FamilyGraphTestStrict, Graph0FindIslands1)
{
	FamilyGraph* graph = buildFamilyGraph(chunkCount0, adjacentChunkPartition0, adjacentChunkIndices0);
	graph->initialize(DEFAULT_ACTOR_INDEX, m_graph);

	std::vector<char> scratch;
	scratch.resize((size_t)FamilyGraph::findIslandsRequiredScratch(chunkCount0));

	graph->notifyEdgeRemoved(DEFAULT_ACTOR_INDEX, 0, 4, m_graph);
	graph->notifyEdgeRemoved(DEFAULT_ACTOR_INDEX, 4, 5, m_graph);
	graph->notifyEdgeRemoved(DEFAULT_ACTOR_INDEX, 1, 2, m_graph);
	EXPECT_EQ(6, graph->getEdgesCount(m_graph));
	EXPECT_EQ(3, graph->findIslands(DEFAULT_ACTOR_INDEX, scratch.data(), m_graph));

	std::vector<IslandInfo> info;
	getIslandsInfo(*graph, info);
	EXPECT_EQ(3, info.size());
	VECTOR_MATCH(info[0].nodes, 4);
	VECTOR_MATCH(info[1].nodes, 0, 1, 5);
	VECTOR_MATCH(info[2].nodes, 2, 3, 6, 7);
}

TEST_F(FamilyGraphTestStrict, Graph0FindIslandsDifferentActors)
{
	const uint32_t ACTOR_0_INDEX = 5;
	const uint32_t ACTOR_1_INDEX = 2;

	FamilyGraph* graph = buildFamilyGraph(chunkCount0, adjacentChunkPartition0, adjacentChunkIndices0);
	graph->initialize(ACTOR_0_INDEX, m_graph);

	std::vector<char> scratch;
	scratch.resize((size_t)FamilyGraph::findIslandsRequiredScratch(chunkCount0));

	EXPECT_EQ(0, graph->findIslands(ACTOR_1_INDEX, scratch.data(), m_graph));
	EXPECT_EQ(1, graph->findIslands(ACTOR_0_INDEX, scratch.data(), m_graph));

	graph->notifyEdgeRemoved(ACTOR_0_INDEX, 2, 1, m_graph);
	EXPECT_EQ(8, graph->getEdgesCount(m_graph));

	EXPECT_EQ(1, graph->findIslands(ACTOR_0_INDEX, scratch.data(), m_graph));

	graph->notifyEdgeRemoved(ACTOR_1_INDEX, 2, 6, m_graph);
	graph->notifyEdgeRemoved(ACTOR_1_INDEX, 7, 3, m_graph);
	EXPECT_EQ(1, graph->findIslands(ACTOR_1_INDEX, scratch.data(), m_graph));

	graph->notifyEdgeRemoved(ACTOR_0_INDEX, 0, 1, m_graph);
	graph->notifyEdgeRemoved(ACTOR_0_INDEX, 4, 5, m_graph);
	EXPECT_EQ(1, graph->findIslands(ACTOR_0_INDEX, scratch.data(), m_graph));


	std::vector<IslandInfo> info;
	getIslandsInfo(*graph, info);
	EXPECT_EQ(4, info.size());
	VECTOR_MATCH(info[0].nodes, 0, 4);
	VECTOR_MATCH(info[1].nodes, 1, 5);
	VECTOR_MATCH(info[2].nodes, 2, 3);
	VECTOR_MATCH(info[3].nodes, 6, 7);
}

TEST_F(FamilyGraphTestStrict, Graph1FindIslands0)
{
	FamilyGraph* graph = buildFamilyGraph(chunkCount1, adjacentChunkPartition1, adjacentChunkIndices1);
	graph->initialize(DEFAULT_ACTOR_INDEX, m_graph);

	std::vector<char> scratch;
	scratch.resize((size_t)FamilyGraph::findIslandsRequiredScratch(chunkCount1));

	graph->notifyEdgeRemoved(DEFAULT_ACTOR_INDEX, 0, 4, m_graph);
	graph->notifyEdgeRemoved(DEFAULT_ACTOR_INDEX, 1, 5, m_graph);
	graph->notifyEdgeRemoved(DEFAULT_ACTOR_INDEX, 2, 6, m_graph);
	graph->notifyEdgeRemoved(DEFAULT_ACTOR_INDEX, 3, 7, m_graph);
	graph->notifyEdgeRemoved(DEFAULT_ACTOR_INDEX, 5, 6, m_graph);
	graph->notifyEdgeRemoved(DEFAULT_ACTOR_INDEX, 9, 10, m_graph);
	EXPECT_EQ(11, graph->getEdgesCount(m_graph));
	EXPECT_EQ(3, graph->findIslands(DEFAULT_ACTOR_INDEX, scratch.data(), m_graph));

	std::vector<IslandInfo> info;
	getIslandsInfo(*graph, info);
	EXPECT_EQ(3, info.size());

	VECTOR_MATCH(info[0].nodes, 0, 1, 2, 3);
	VECTOR_MATCH(info[1].nodes, 4, 5, 8, 9);
	VECTOR_MATCH(info[2].nodes, 6, 7, 10, 11);
}

TEST_F(FamilyGraphTestStrict, Graph1FindIslands1)
{
	FamilyGraph* graph = buildFamilyGraph(chunkCount1, adjacentChunkPartition1, adjacentChunkIndices1);
	graph->initialize(DEFAULT_ACTOR_INDEX, m_graph);

	std::vector<char> scratch;
	scratch.resize((size_t)FamilyGraph::findIslandsRequiredScratch(chunkCount1));

	graph->notifyEdgeRemoved(DEFAULT_ACTOR_INDEX, 0, 4, m_graph);
	EXPECT_EQ(1, graph->findIslands(DEFAULT_ACTOR_INDEX, scratch.data(), m_graph));
	graph->notifyEdgeRemoved(DEFAULT_ACTOR_INDEX, 1, 5, m_graph);
	EXPECT_EQ(0, graph->findIslands(DEFAULT_ACTOR_INDEX, scratch.data(), m_graph));
	graph->notifyEdgeRemoved(DEFAULT_ACTOR_INDEX, 2, 6, m_graph);
	EXPECT_EQ(0, graph->findIslands(DEFAULT_ACTOR_INDEX, scratch.data(), m_graph));
	graph->notifyEdgeRemoved(DEFAULT_ACTOR_INDEX, 3, 7, m_graph);
	EXPECT_EQ(1, graph->findIslands(DEFAULT_ACTOR_INDEX, scratch.data(), m_graph));
	graph->notifyEdgeRemoved(DEFAULT_ACTOR_INDEX, 5, 6, m_graph);
	EXPECT_EQ(0, graph->findIslands(DEFAULT_ACTOR_INDEX, scratch.data(), m_graph));
	graph->notifyEdgeRemoved(DEFAULT_ACTOR_INDEX, 9, 10, m_graph);
	EXPECT_EQ(1, graph->findIslands(DEFAULT_ACTOR_INDEX, scratch.data(), m_graph));

	std::vector<IslandInfo> info;
	getIslandsInfo(*graph, info);
	EXPECT_EQ(3, info.size());
	VECTOR_MATCH(info[0].nodes, 0, 1, 2, 3);
	VECTOR_MATCH(info[1].nodes, 4, 5, 8, 9);
	VECTOR_MATCH(info[2].nodes, 6, 7, 10, 11);
}

TEST_F(FamilyGraphTestStrict, Graph1FindIslandsRemoveAllEdges)
{
	FamilyGraph* graph = buildFamilyGraph(chunkCount1, adjacentChunkPartition1, adjacentChunkIndices1);
	graph->initialize(DEFAULT_ACTOR_INDEX, m_graph);

	std::vector<char> scratch;
	scratch.resize((size_t)FamilyGraph::findIslandsRequiredScratch(chunkCount1));

	uint32_t edges = graph->getEdgesCount(m_graph);
	for (uint32_t node0 = 0; node0 < chunkCount1; node0++)
	{
		for (uint32_t i = adjacentChunkPartition1[node0]; i < adjacentChunkPartition1[node0 + 1]; i++)
		{
			if (graph->notifyEdgeRemoved(DEFAULT_ACTOR_INDEX, node0, adjacentChunkIndices1[i], m_graph))
			{
				edges--;
				EXPECT_EQ(edges, graph->getEdgesCount(m_graph));
			}
		}
	}
	EXPECT_EQ(0, graph->getEdgesCount(m_graph));

	EXPECT_EQ(12, graph->findIslands(DEFAULT_ACTOR_INDEX, scratch.data(), m_graph));

	for (uint32_t node0 = 0; node0 < chunkCount1; node0++)
	{
		EXPECT_EQ(node0, graph->getIslandIds()[node0]);
	}
}
