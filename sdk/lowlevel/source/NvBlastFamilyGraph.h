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


#ifndef NVBLASTFAMILYGRAPH_H
#define NVBLASTFAMILYGRAPH_H


#include "NvBlastSupportGraph.h"
#include "NvBlastFixedArray.h"
#include "NvBlastFixedBitmap.h"
#include "NvBlastFixedBoolArray.h"
#include "NvBlastMath.h"
#include "NvBlastFixedPriorityQueue.h"
#include "NvBlastMemory.h"


namespace Nv
{
namespace Blast
{


typedef uint32_t NodeIndex;
typedef NodeIndex IslandId;
typedef uint32_t ActorIndex;

/**
Internal implementation of family graph stored on the family.

It processes full NvBlastSupportGraph graph, stores additional information used for faster islands finding, 
keeps and provides access to current islandId for every node.
*/
class FamilyGraph
{
public:

	//////// ctor ////////

	/**
	Constructor. family graph is meant to be placed (with placement new) on family memory.

	\param[in] graph	The graph to instance (see SupportGraph)
	*/
	FamilyGraph(const SupportGraph* graph);


	/**
	Returns memory needed for this class (see fillMemory).

	\param[in] nodeCount	The number of nodes in the graph.
	\param[in] bondCount	The number of bonds in the graph.

	\return the number of bytes required.
	*/
	static size_t	requiredMemorySize(uint32_t nodeCount, uint32_t bondCount)
	{
		return fillMemory(nullptr, nodeCount, bondCount);
	}


	//////// API ////////

	/**
	Function to initialize graph (all nodes added to dirty list for this actor)

	\param[in] actorIndex	The index of the actor to initialize graph with. Must be in the range [0, m_nodeCount).
	\param[in] graph		The static graph data for this family.
	*/
	void			initialize(ActorIndex actorIndex, const SupportGraph* graph);

	/**
	Function to notify graph about removed edges. These nodes will be added to dirty list for this actor. Returns true if bond as removed.

	\param[in] actorIndex	The index of the actor from which the edge is removed. Must be in the range [0, m_nodeCount).
	\param[in] node0		The index of the first node of removed edge. Must be in the range [0, m_nodeCount).
	\param[in] node1		The index of the second node of removed edge. Must be in the range [0, m_nodeCount).
	\param[in] graph		The static graph data for this family.
	*/
	bool			notifyEdgeRemoved(ActorIndex actorIndex, NodeIndex node0, NodeIndex node1, const SupportGraph* graph);
	bool			notifyEdgeRemoved(ActorIndex actorIndex, NodeIndex node0, NodeIndex node1, uint32_t bondIndex, const SupportGraph* graph);

	bool			notifyNodeRemoved(ActorIndex actorIndex, NodeIndex nodeIndex, const SupportGraph* graph);

	/**
	Function to find new islands by examining dirty nodes associated with this actor (they can be associated with actor if 
	notifyEdgeRemoved() were previously called for it.

	\param[in] actorIndex	The index of the actor on which graph part (edges + nodes) findIslands will be performed. Must be in the range [0, m_nodeCount).
	\param[in] scratch		User-supplied scratch memory of size findIslandsRequiredScratch(graphNodeCount) bytes.
	\param[in] graph		The static graph data for this family.

	\return the number of new islands found.
	*/
	uint32_t		findIslands(ActorIndex actorIndex, void* scratch, const SupportGraph* graph);

	/**
	The scratch space required to call the findIslands function, in bytes.

	\param[in] graphNodeCount The number of nodes in the graph.

	\return the number of bytes required.
	*/
	static size_t	findIslandsRequiredScratch(uint32_t graphNodeCount);


	//////// data getters ////////

	/**
	Utility function to get the start of the island ids array. This is an array of size nodeCount.
	Every islandId == NodeIndex of root node in this island, it is set for every Node.

	\return the array of island ids.
	*/
	NvBlastBlockData(IslandId, m_islandIdsOffset, getIslandIds);

	/**
	Utility function to get the start of the dirty node links array. This is an array of size nodeCount.
	*/
	NvBlastBlockData(NodeIndex, m_dirtyNodeLinksOffset, getDirtyNodeLinks);

	/**
	Utility function to get the start of the first dirty node indices array. This is an array of size nodeCount.
	*/
	NvBlastBlockData(uint32_t, m_firstDirtyNodeIndicesOffset, getFirstDirtyNodeIndices);

	/**
	Utility function to get the start of the fast route array. This is an array of size nodeCount.
	*/
	NvBlastBlockData(NodeIndex, m_fastRouteOffset, getFastRoute);

	/**
	Utility function to get the start of the hop counts array. This is an array of size nodeCount.
	*/
	NvBlastBlockData(uint32_t, m_hopCountsOffset, getHopCounts);

	/**
	Utility function to get the pointer of the is edge removed bitmap. This is an bitmap of size bondCount.
	*/
	NvBlastBlockData(FixedBoolArray, m_isEdgeRemovedOffset, getIsEdgeRemoved);

	/**
	Utility function to get the pointer of the is node in dirty list bitmap. This is an bitmap of size nodeCount.
	*/
	NvBlastBlockData(FixedBoolArray, m_isNodeInDirtyListOffset, getIsNodeInDirtyList);


	//////// Debug/Test ////////

	uint32_t	getEdgesCount(const SupportGraph* graph) const;
	bool		hasEdge(NodeIndex node0, NodeIndex node1, const SupportGraph* graph) const;
	bool		canFindRoot(NodeIndex startNode, NodeIndex targetNode, FixedArray<NodeIndex>* visitedNodes, const SupportGraph* graph);


private:

	FamilyGraph& operator = (const FamilyGraph&);

	//////// internal types ////////

	/**
	Used to represent current graph traverse state.
	*/
	struct TraversalState
	{
		NodeIndex mNodeIndex;
		uint32_t mCurrentIndex;
		uint32_t mPrevIndex;
		uint32_t mDepth;

		TraversalState()
		{
		}

		TraversalState(NodeIndex nodeIndex, uint32_t currentIndex, uint32_t prevIndex, uint32_t depth) :
			mNodeIndex(nodeIndex), mCurrentIndex(currentIndex), mPrevIndex(prevIndex), mDepth(depth)
		{
		}
	};

	/**
	Queue element for graph traversal with priority queue.
	*/
	struct QueueElement
	{
		TraversalState* mState;
		uint32_t mHopCount;

		QueueElement()
		{
		}

		QueueElement(TraversalState* state, uint32_t hopCount) : mState(state), mHopCount(hopCount)
		{
		}
	};

	/**
	Queue comparator for graph traversal with priority queue.
	*/
	struct NodeComparator
	{
		NodeComparator()
		{
		}

		bool operator() (const QueueElement& node0, const QueueElement& node1) const
		{
			return node0.mHopCount < node1.mHopCount;
		}
	private:
		NodeComparator& operator = (const NodeComparator&);
	};

	/**
	PriorityQueue for graph traversal. Queue element with smallest hopCounts will be always on top.
	*/
	typedef FixedPriorityQueue<QueueElement, NodeComparator> NodePriorityQueue;


	//////// internal operations ////////

	/**
	Function calculate needed memory and feel it if familyGraph is passed. FamilyGraph is designed to use 
	memory right after itself. So it should be initialized with placement new operation	on memory of memoryNeeded() size.

	\param[in] familyGraph		The pointer to actual FamilyGraph instance which will be filled. Can be nullptr, function will only return required bytes and do nothing.
	\param[in] nodeCount		The number of nodes in the graph.
	\param[in] bondCount		The number of bonds in the graph.

	\return the number of bytes required or filled
	*/
	static size_t	fillMemory(FamilyGraph* familyGraph, uint32_t nodeCount, uint32_t bondCount);

	/**
	Function to find route from on node to another. It uses fastPath first as optimization and then if it fails it performs brute-force traverse (with hop count heuristic)
	*/
	bool			findRoute(NodeIndex startNode, NodeIndex targetNode, IslandId islandId, FixedArray<TraversalState>* visitedNodes, FixedBitmap* isNodeWitness, NodePriorityQueue* priorityQueue, const SupportGraph* graph);

	/**
	Function to try finding targetNode (from startNode) with getFastRoute().
	*/
	bool			tryFastPath(NodeIndex startNode, NodeIndex targetNode, IslandId islandId, FixedArray<TraversalState>* visitedNodes, FixedBitmap* isNodeWitness, const SupportGraph* graph);

	/**
	Function to unwind route upon successful finding of root node or witness.
	We have found either a witness *or* the root node with this traversal. In the event of finding the root node, hopCount will be 0. In the event of finding
	a witness, hopCount will be the hopCount that witness reported as being the distance to the root.
	*/
	void			unwindRoute(uint32_t traversalIndex, NodeIndex lastNode, uint32_t hopCount, IslandId id, FixedArray<TraversalState>* visitedNodes);

	/**
	Function to add node to dirty node list associated with actor.
	*/
	void			addToDirtyNodeList(ActorIndex actorIndex, NodeIndex node);

	/**
	Function used to get adjacentNode using index from adjacencyPartition with check for bondHealths (if it's not removed already)
	*/
	NodeIndex		getAdjacentNode(uint32_t adjacencyIndex, const SupportGraph* graph) const
	{
		const uint32_t bondIndex = graph->getAdjacentBondIndices()[adjacencyIndex];
		return getIsEdgeRemoved()->test(bondIndex) ? invalidIndex<uint32_t>() : graph->getAdjacentNodeIndices()[adjacencyIndex];
	}

};


} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTFAMILYGRAPH_H
