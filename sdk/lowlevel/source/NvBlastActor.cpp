/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NvBlastActor.h"
#include "NvBlastFamilyGraph.h"
#include "NvBlastChunkHierarchy.h"
#include "NvBlastIndexFns.h"
#include "NvBlastDLink.h"
#include "NvBlastGeometry.h"
#include "NvBlastTime.h"
#include <float.h>
#include <algorithm>


namespace Nv
{
namespace Blast
{

//////// Local helper functions ////////

#if NVBLAST_CHECK_PARAMS
/**
Helper function to validate fracture buffer values being meaningful.
*/
static inline bool isValid(const NvBlastFractureBuffers* buffers)
{
	if (buffers->chunkFractureCount != 0 && buffers->chunkFractures == nullptr)
		return false;

	if (buffers->bondFractureCount != 0 && buffers->bondFractures == nullptr)
		return false;

	return true;
}
#endif

//////// Actor static methods ////////

size_t Actor::createRequiredScratch(const NvBlastFamily* family)
{
#if NVBLAST_CHECK_PARAMS
	if (family == nullptr || reinterpret_cast<const FamilyHeader*>(family)->m_asset == nullptr)
	{
		NVBLAST_ALWAYS_ASSERT();
		return 0;
	}
#endif
	
	const Asset& solverAsset = *reinterpret_cast<const FamilyHeader*>(family)->m_asset;
	return FamilyGraph::findIslandsRequiredScratch(solverAsset.m_graph.m_nodeCount);
}


Actor* Actor::create(NvBlastFamily* family, const NvBlastActorDesc* desc, void* scratch, NvBlastLog logFn)
{
	NVBLAST_CHECK(family != nullptr, logFn, "Actor::create: NULL family pointer input.", return nullptr);
	NVBLAST_CHECK(reinterpret_cast<FamilyHeader*>(family)->m_asset != nullptr, logFn, "Actor::create: family has NULL asset.", return nullptr);
	NVBLAST_CHECK(reinterpret_cast<FamilyHeader*>(family)->m_asset->m_graph.m_nodeCount != 0, logFn, "Actor::create: family's asset has no support chunks.", return nullptr);
	NVBLAST_CHECK(desc != nullptr, logFn, "Actor::create: NULL desc pointer input.", return nullptr);
	NVBLAST_CHECK(scratch != nullptr, logFn, "Actor::create: NULL scratch input.", return nullptr);

	FamilyHeader* header = reinterpret_cast<FamilyHeader*>(family);

	if (header->m_actorCount > 0)
	{
		NVBLAST_LOG_ERROR(logFn, "Actor::create: input family is not empty.");
		return nullptr;
	}

	const Asset& solverAsset = *static_cast<const Asset*>(header->m_asset);
	const Nv::Blast::SupportGraph& graph = solverAsset.m_graph;

	// Lower support chunk healths - initialize
	float* lowerSupportChunkHealths = header->getLowerSupportChunkHealths();
	if (desc->initialSupportChunkHealths != nullptr)	// Health array given
	{
		const uint32_t* supportChunkIndices = graph.getChunkIndices();
		for (uint32_t supportChunkNum = 0; supportChunkNum < graph.m_nodeCount; ++supportChunkNum)
		{
			const float initialHealth = desc->initialSupportChunkHealths[supportChunkNum];
			for (Asset::DepthFirstIt i(solverAsset, supportChunkIndices[supportChunkNum]); (bool)i; ++i)
			{
				lowerSupportChunkHealths[solverAsset.getContiguousLowerSupportIndex((uint32_t)i)] = initialHealth;
			}
		}
	}
	else	// Use uniform initialization
	{
		const uint32_t lowerSupportChunkCount = solverAsset.getLowerSupportChunkCount();
		for (uint32_t i = 0; i < lowerSupportChunkCount; ++i)
		{
			lowerSupportChunkHealths[i] = desc->uniformInitialLowerSupportChunkHealth;
		}
	}

	// Bond healths - initialize
	const uint32_t bondCount = solverAsset.getBondCount();
	float* bondHealths = header->getBondHealths();
	if (desc->initialBondHealths != nullptr)	// Health array given
	{
		memcpy(bondHealths, desc->initialBondHealths, bondCount * sizeof(float));
	}
	else	// Use uniform initialization
	{
		for (uint32_t bondNum = 0; bondNum < bondCount; ++bondNum)
		{
			bondHealths[bondNum] = desc->uniformInitialBondHealth;
		}
	}

	// Get first actor - NOTE: we don't send an event for this!  May need to do so for consistency.
	Actor* actor = header->borrowActor(0);	// Using actor[0]

	// Fill in actor fields
	actor->m_firstGraphNodeIndex = 0;
	actor->m_graphNodeCount = graph.m_nodeCount;
	actor->m_leafChunkCount = solverAsset.m_leafChunkCount;

	// Graph node index links - initialize to chain
	uint32_t* graphNodeLinks = header->getGraphNodeIndexLinks();
	for (uint32_t i = 0; i < graph.m_nodeCount - 1; ++i)
	{
		graphNodeLinks[i] = i + 1;
	}
	graphNodeLinks[graph.m_nodeCount - 1] = invalidIndex<uint32_t>();

	// Update visible chunks (we assume that all chunks belong to one actor at the beginning)
	actor->updateVisibleChunksFromGraphNodes();

	// Initialize instance graph with this actor
	header->getFamilyGraph()->initialize(actor->getIndex(), &graph);

	// Call findIslands to set up the internal instance graph data
	header->getFamilyGraph()->findIslands(actor->getIndex(), scratch, &graph);

	return actor;
}


//////// Actor member methods ////////

uint32_t Actor::damageBond(uint32_t nodeIndex0, uint32_t nodeIndex1, float healthDamage)
{
	const uint32_t bondIndex = getGraph()->findBond(nodeIndex0, nodeIndex1);
	damageBond(nodeIndex0, nodeIndex1, bondIndex, healthDamage);
	return bondIndex;
}


void Actor::damageBond(uint32_t nodeIndex0, uint32_t nodeIndex1, uint32_t bondIndex, float healthDamage)
{
	if (bondIndex == invalidIndex<uint32_t>())
	{
		NVBLAST_ALWAYS_ASSERT();
		return;
	}

	float* bondHealths = getBondHealths();
	if (bondHealths[bondIndex] > 0 && healthDamage > 0.0f)
	{
		// Subtract health
		bondHealths[bondIndex] -= healthDamage;

		// Was removed?
		if (bondHealths[bondIndex] <= 0)
		{
			// Notify graph that bond was removed
			getFamilyGraph()->notifyEdgeRemoved(getIndex(), nodeIndex0, nodeIndex1, bondIndex, getGraph());
			bondHealths[bondIndex] = 0;	// Doing this for single-actor serialization consistency; should not actually be necessary
		}
	}
}


uint32_t Actor::damageBond(const NvBlastBondFractureData& cmd)
{
	NVBLAST_ASSERT(!isInvalidIndex(cmd.nodeIndex1));
	return damageBond(cmd.nodeIndex0, cmd.nodeIndex1, cmd.health);
}


void Actor::generateFracture(NvBlastFractureBuffers* commandBuffers, const NvBlastDamageProgram& program, const NvBlastProgramParams* programParams, 
	NvBlastLog logFn, NvBlastTimers* timers) const
{
	NVBLAST_CHECK(commandBuffers != nullptr, logFn, "Actor::generateFracture: NULL commandBuffers pointer input.", return);
	NVBLAST_CHECK(isValid(commandBuffers), logFn, "NvBlastActorGenerateFracture: commandBuffers memory is NULL but size is > 0.",
		commandBuffers->bondFractureCount = 0; commandBuffers->chunkFractureCount = 0; return);

#if NVBLAST_CHECK_PARAMS
	if (commandBuffers->bondFractureCount == 0 && commandBuffers->chunkFractureCount == 0)
	{
		NVBLAST_LOG_WARNING(logFn, "NvBlastActorGenerateFracture: commandBuffers do not provide any space.");
		return;
	}
#endif

#if NV_PROFILE
	Time time;
#else
	NV_UNUSED(timers);
#endif

	const SupportGraph* graph = getGraph();

	const uint32_t graphNodeCount = getGraphNodeCount();
	if (graphNodeCount > 1 && program.graphShaderFunction != nullptr)
	{
		const NvBlastGraphShaderActor shaderActor = {
			getFirstGraphNodeIndex(),
			getGraphNodeIndexLinks(),
			graph->getChunkIndices(),
			graph->getAdjacencyPartition(),
			graph->getAdjacentNodeIndices(),
			graph->getAdjacentBondIndices(),
			getBonds(),
			getBondHealths()
		};

		program.graphShaderFunction(commandBuffers, &shaderActor, programParams);
	}
	else if (graphNodeCount <= 1 && program.subgraphShaderFunction != nullptr)
	{
		const NvBlastSubgraphShaderActor shaderActor = {
			// The conditional (visible vs. support chunk) is needed because we allow single-child chunk chains
			// This makes it possible that an actor with a single support chunk will have a different visible chunk (ancestor of the support chunk)
			graphNodeCount == 1 ? graph->getChunkIndices()[getFirstGraphNodeIndex()] : getFirstVisibleChunkIndex(),
			getChunks()
		};

		program.subgraphShaderFunction(commandBuffers, &shaderActor, programParams);
	}
	else
	{
		commandBuffers->bondFractureCount = 0;
		commandBuffers->chunkFractureCount = 0;
	}

#if NV_PROFILE
	if (timers != nullptr)
	{
		timers->material += time.getElapsedTicks();
	}
#endif
}


void Actor::fractureSubSupportNoEvents(uint32_t chunkIndex, uint32_t suboffset, float healthDamage, float* chunkHealths, const NvBlastChunk* chunks)
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


void Actor::fractureSubSupport(uint32_t chunkIndex, uint32_t suboffset, float healthDamage, float* chunkHealths, const NvBlastChunk* chunks, NvBlastChunkFractureData* outBuffer, uint32_t* currentIndex, const uint32_t maxCount)
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


void Actor::fractureNoEvents(uint32_t chunkFractureCount, const NvBlastChunkFractureData* chunkFractures)
{
	const Asset& asset = *getAsset();
	const SupportGraph& graph = *getGraph();
	const uint32_t* graphAdjacencyPartition = graph.getAdjacencyPartition();
	const uint32_t* graphAdjacentNodeIndices = graph.getAdjacentNodeIndices();
	float* bondHealths = getBondHealths();
	float* chunkHealths = getLowerSupportChunkHealths();
	float* subChunkHealths = getSubsupportChunkHealths();
	const NvBlastChunk* chunks = getChunks();

	for (uint32_t i = 0; i < chunkFractureCount; ++i)
	{
		const NvBlastChunkFractureData& command = chunkFractures[i];
		const uint32_t chunkIndex = command.chunkIndex;
		const uint32_t chunkHealthIndex = asset.getContiguousLowerSupportIndex(chunkIndex);
		NVBLAST_ASSERT(!isInvalidIndex(chunkHealthIndex));
		if (isInvalidIndex(chunkHealthIndex))
		{
			continue;
		}
		float& health = chunkHealths[chunkHealthIndex];
		if (health > 0.0f && command.health > 0.0f)
		{
			const uint32_t nodeIndex = asset.getChunkToGraphNodeMap()[chunkIndex];
			if (getGraphNodeCount() > 1 && !isInvalidIndex(nodeIndex))
			{
				for (uint32_t adjacentIndex = graphAdjacencyPartition[nodeIndex]; adjacentIndex < graphAdjacencyPartition[nodeIndex + 1]; adjacentIndex++)
				{

					const uint32_t bondIndex = graph.findBond(nodeIndex, graphAdjacentNodeIndices[adjacentIndex]);
					NVBLAST_ASSERT(!isInvalidIndex(bondIndex));
					if (bondHealths[bondIndex] > 0.0f)
					{
						bondHealths[bondIndex] = 0.0f;
					}
				}
				getFamilyGraph()->notifyNodeRemoved(getIndex(), nodeIndex, &graph);
			}

			health -= command.health;

			const float remainingDamage = -health;

			if (remainingDamage > 0.0f) // node chunk has been damaged beyond its health
			{
				uint32_t firstSubOffset = getFirstSubsupportChunkIndex();
				fractureSubSupportNoEvents(chunkIndex, firstSubOffset, remainingDamage, subChunkHealths, chunks);
			}
		}
	}
}


void Actor::fractureWithEvents(uint32_t chunkFractureCount, const NvBlastChunkFractureData* commands, NvBlastChunkFractureData* events, uint32_t eventsSize, uint32_t* count)
{
	const Asset& asset = *getAsset();
	const SupportGraph& graph = *getGraph();
	const uint32_t* graphAdjacencyPartition = graph.getAdjacencyPartition();
	const uint32_t* graphAdjacentNodeIndices = graph.getAdjacentNodeIndices();
	float* bondHealths = getBondHealths();
	float* chunkHealths = getLowerSupportChunkHealths();
	float* subChunkHealths = getSubsupportChunkHealths();
	const NvBlastChunk* chunks = getChunks();

	for (uint32_t i = 0; i < chunkFractureCount; ++i)
	{
		const NvBlastChunkFractureData& command = commands[i];
		const uint32_t chunkIndex = command.chunkIndex;
		const uint32_t chunkHealthIndex = asset.getContiguousLowerSupportIndex(chunkIndex);
		NVBLAST_ASSERT(!isInvalidIndex(chunkHealthIndex));
		if (isInvalidIndex(chunkHealthIndex))
		{
			continue;
		}
		float& health = chunkHealths[chunkHealthIndex];
		if (health > 0.0f && command.health > 0.0f)
		{
			const uint32_t nodeIndex = asset.getChunkToGraphNodeMap()[chunkIndex];
			if (getGraphNodeCount() > 1 && !isInvalidIndex(nodeIndex))
			{
				for (uint32_t adjacentIndex = graphAdjacencyPartition[nodeIndex]; adjacentIndex < graphAdjacencyPartition[nodeIndex + 1]; adjacentIndex++)
				{
					const uint32_t bondIndex = graph.findBond(nodeIndex, graphAdjacentNodeIndices[adjacentIndex]);
					NVBLAST_ASSERT(!isInvalidIndex(bondIndex));
					if (bondHealths[bondIndex] > 0.0f)
					{
						bondHealths[bondIndex] = 0.0f;
					}
				}
				getFamilyGraph()->notifyNodeRemoved(getIndex(), nodeIndex, &graph);
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
				uint32_t firstSubOffset = getFirstSubsupportChunkIndex();
				fractureSubSupport(chunkIndex, firstSubOffset, remainingDamage, subChunkHealths, chunks, events, count, eventsSize);
			}
		}
	}
}


void Actor::fractureInPlaceEvents(uint32_t chunkFractureCount, NvBlastChunkFractureData* inoutbuffer, uint32_t eventsSize, uint32_t* count)
{
	const Asset& asset = *getAsset();
	const SupportGraph& graph = *getGraph();
	const uint32_t* graphAdjacencyPartition = graph.getAdjacencyPartition();
	const uint32_t* graphAdjacentNodeIndices = graph.getAdjacentNodeIndices();
	float* bondHealths = getBondHealths();
	float* chunkHealths = getLowerSupportChunkHealths();
	float* subChunkHealths = getSubsupportChunkHealths();
	const NvBlastChunk* chunks = getChunks();

	//
	// First level Chunk Fractures
	//

	for (uint32_t i = 0; i < chunkFractureCount; ++i)
	{
		const NvBlastChunkFractureData& command = inoutbuffer[i];
		const uint32_t chunkIndex = command.chunkIndex;
		const uint32_t chunkHealthIndex = asset.getContiguousLowerSupportIndex(chunkIndex);
		NVBLAST_ASSERT(!isInvalidIndex(chunkHealthIndex));
		if (isInvalidIndex(chunkHealthIndex))
		{
			continue;
		}
		float& health = chunkHealths[chunkHealthIndex];
		if (health > 0.0f && command.health > 0.0f)
		{
			const uint32_t nodeIndex = asset.getChunkToGraphNodeMap()[chunkIndex];
			if (getGraphNodeCount() > 1 && !isInvalidIndex(nodeIndex))
			{
				for (uint32_t adjacentIndex = graphAdjacencyPartition[nodeIndex]; adjacentIndex < graphAdjacencyPartition[nodeIndex + 1]; adjacentIndex++)
				{
					const uint32_t bondIndex = graph.findBond(nodeIndex, graphAdjacentNodeIndices[adjacentIndex]);
					NVBLAST_ASSERT(!isInvalidIndex(bondIndex));
					if (bondHealths[bondIndex] > 0.0f)
					{
						bondHealths[bondIndex] = 0.0f;
					}
				}
				getFamilyGraph()->notifyNodeRemoved(getIndex(), nodeIndex, &graph);
			}

			health -= command.health;

			NvBlastChunkFractureData& outEvent = inoutbuffer[(*count)++];
			outEvent.userdata = chunks[chunkIndex].userData;
			outEvent.chunkIndex = chunkIndex;
			outEvent.health = health;
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
			uint32_t firstSubOffset = getFirstSubsupportChunkIndex();
			fractureSubSupport(chunkIndex, firstSubOffset, remainingDamage, subChunkHealths, chunks, inoutbuffer, count, eventsSize);
		}
	}
}


void Actor::applyFracture(NvBlastFractureBuffers* eventBuffers, const NvBlastFractureBuffers* commands, NvBlastLog logFn, NvBlastTimers* timers)
{
	NVBLAST_CHECK(commands != nullptr, logFn, "Actor::applyFracture: NULL commands pointer input.", return);
	NVBLAST_CHECK(isValid(commands), logFn, "Actor::applyFracture: commands memory is NULL but size is > 0.", return);
	NVBLAST_CHECK(eventBuffers == nullptr || isValid(eventBuffers), logFn, "NvBlastActorApplyFracture: eventBuffers memory is NULL but size is > 0.",
		eventBuffers->bondFractureCount = 0; eventBuffers->chunkFractureCount = 0; return);

#if NVBLAST_CHECK_PARAMS
	if (eventBuffers != nullptr && eventBuffers->bondFractureCount == 0 && eventBuffers->chunkFractureCount == 0)
	{
		NVBLAST_LOG_WARNING(logFn, "NvBlastActorApplyFracture: eventBuffers do not provide any space.");
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
		fractureNoEvents(commands->chunkFractureCount, commands->chunkFractures);
	}
	else if (eventBuffers->chunkFractures != commands->chunkFractures)
	{
		// immediate hierarchical fracture
		uint32_t count = 0;
		fractureWithEvents(commands->chunkFractureCount, commands->chunkFractures, eventBuffers->chunkFractures, eventBuffers->chunkFractureCount, &count);

		if (count > eventBuffers->chunkFractureCount)
		{
			NVBLAST_LOG_WARNING(logFn, "NvBlastActorApplyFracture: eventBuffers too small. Chunk events were lost.");
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
		fractureInPlaceEvents(commands->chunkFractureCount, commands->chunkFractures, eventBuffers->chunkFractureCount, &count);

		if (count > eventBuffers->chunkFractureCount)
		{
			NVBLAST_LOG_WARNING(logFn, "NvBlastActorApplyFracture: eventBuffers too small. Chunk events were lost.");
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

	NvBlastBond* bonds = getBonds();
	float* bondHealths = getBondHealths();
	for (uint32_t i = 0; i < commands->bondFractureCount; ++i)
	{
		const NvBlastBondFractureData& frac = commands->bondFractures[i];

		const uint32_t bondIndex = damageBond(frac.nodeIndex0, frac.nodeIndex1, frac.health);

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

	if (eventBuffers && eventBuffers->bondFractures)
	{
		if (outCount > eventBufferSize)
		{
			NVBLAST_LOG_WARNING(logFn, "NvBlastActorApplyFracture: eventBuffers too small. Bond events were lost.");
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


size_t Actor::splitRequiredScratch() const
{
	return FamilyGraph::findIslandsRequiredScratch(getGraph()->m_nodeCount);
}


uint32_t Actor::split(NvBlastActorSplitEvent* result, uint32_t newActorsMaxCount, void* scratch, NvBlastLog logFn, NvBlastTimers* timers)
{
	NVBLAST_CHECK(result != nullptr, logFn, "Actor::split: NULL result pointer input.", return 0);
	NVBLAST_CHECK(newActorsMaxCount > 0 && result->newActors != nullptr, logFn, "NvBlastActorSplit: no space for results provided.", return 0);
	NVBLAST_CHECK(scratch != nullptr, logFn, "Actor::split: NULL scratch pointer input.", return 0);

#if NV_PROFILE
	Time time;
#else
	NV_UNUSED(timers);
#endif

	Actor** newActors = reinterpret_cast<Actor**>(result->newActors);

	uint32_t actorsCount = 0;

	if (getGraphNodeCount() <= 1)
	{
		uint32_t chunkHealthIndex = isSingleSupportChunk() ? getIndex() : getFirstVisibleChunkIndex() - getFirstSubsupportChunkIndex() + getGraph()->m_nodeCount;

		float* chunkHealths = getLowerSupportChunkHealths();
		if (chunkHealths[chunkHealthIndex] <= 0.0f)
		{
			actorsCount = partitionSingleLowerSupportChunk(newActors, newActorsMaxCount, logFn);

			for (uint32_t i = 0; i < actorsCount; ++i)
			{
				Actor* newActor = newActors[i];
				uint32_t firstVisible = newActor->getFirstVisibleChunkIndex();
				uint32_t firstSub = newActor->getFirstSubsupportChunkIndex();
				uint32_t nodeCount = newActor->getGraph()->m_nodeCount;
				uint32_t newActorIndex = newActor->getIndex();
				uint32_t healthIndex = newActor->isSubSupportChunk() ? firstVisible - firstSub + nodeCount : newActorIndex;

				if (chunkHealths[healthIndex] <= 0.0f)
				{
					uint32_t brittleActors = newActors[i]->partitionSingleLowerSupportChunk(&newActors[actorsCount], newActorsMaxCount - actorsCount, logFn);
					actorsCount += brittleActors;

					if (brittleActors > 0)
					{
						actorsCount--;
						newActors[i] = newActors[actorsCount];
						i--;
					}
				}
			}
		}


#if NV_PROFILE
		if (timers != nullptr)
		{
			timers->partition += time.getElapsedTicks();
		}
#endif
	}
	else
	{
		findIslands(scratch);

#if NV_PROFILE
		if (timers != nullptr)
		{
			timers->island += time.getElapsedTicks();
		}
#endif

		actorsCount = partitionMultipleGraphNodes(newActors, newActorsMaxCount, logFn);

		if (actorsCount > 1)
		{
#if NV_PROFILE
			if (timers != nullptr)
			{
				timers->partition += time.getElapsedTicks();
			}
#endif

			// Recalculate visible chunk lists if the graph nodes have been redistributed
			for (uint32_t i = 0; i < actorsCount; ++i)
			{
				newActors[i]->updateVisibleChunksFromGraphNodes();
			}

#if NV_PROFILE
			if (timers != nullptr)
			{
				timers->visibility += time.getElapsedTicks();
			}
#endif

			for (uint32_t i = 0; i < actorsCount; ++i)
			{
				Actor* newActor = newActors[i];
				float* chunkHealths = newActor->getLowerSupportChunkHealths();
				uint32_t firstVisible = newActor->getFirstVisibleChunkIndex();
				uint32_t firstSub = newActor->getFirstSubsupportChunkIndex();
				uint32_t nodeCount = newActor->getGraph()->m_nodeCount;
				uint32_t newActorIndex = newActor->getIndex();
				uint32_t healthIndex = newActor->isSubSupportChunk() ? firstVisible - firstSub + nodeCount : newActorIndex;

				if (newActors[i]->getGraphNodeCount() <= 1)
				{
					// this relies on visibility updated, subsupport actors only have m_firstVisibleChunkIndex to identify the chunk
					if (chunkHealths[healthIndex] <= 0.0f)
					{
						uint32_t brittleActors = newActors[i]->partitionSingleLowerSupportChunk(&newActors[actorsCount], newActorsMaxCount - actorsCount, logFn);
						actorsCount += brittleActors;

						if (brittleActors > 0)
						{
							actorsCount--;
							newActors[i] = newActors[actorsCount];
							i--;
						}
					}
				}
			}

#if NV_PROFILE
			if (timers != nullptr)
			{
				timers->partition += time.getElapsedTicks();
			}
#endif
		}
		else
		{
			actorsCount = 0;
		}
	}

	result->deletedActor = actorsCount == 0 ? nullptr : this;

	return actorsCount;
}
	
	
uint32_t Actor::findIslands(void* scratch)
{
	return getFamilyHeader()->getFamilyGraph()->findIslands(getIndex(), scratch, &getAsset()->m_graph);
}


uint32_t Actor::partitionMultipleGraphNodes(Actor** newActors, uint32_t newActorsSize, NvBlastLog logFn)
{
	NVBLAST_ASSERT(newActorsSize == 0 || newActors != nullptr);

	// Check for single subsupport chunk, no partitioning
	if (m_graphNodeCount <= 1)
	{
		NVBLAST_LOG_WARNING(logFn, "Nv::Blast::Actor::partitionMultipleGraphNodes: actor is a single lower-support chunk, and cannot be partitioned by this function.");
		return 0;
	}

	FamilyHeader* header = getFamilyHeader();
	NVBLAST_ASSERT(header != nullptr);	// If m_actorEntryDataIndex is valid, this should be too

	// Get the links for the graph nodes
	uint32_t* graphNodeIndexLinks = header->getGraphNodeIndexLinks();

	// Get the graph chunk indices and leaf chunk counts
	const Asset* asset = getAsset();
	const uint32_t* graphChunkIndices = asset->m_graph.getChunkIndices();
	const uint32_t* subtreeLeafChunkCounts = asset->getSubtreeLeafChunkCounts();

	// Distribute graph nodes to new actors
	uint32_t newActorCount = 0;
	const uint32_t thisActorIndex = getIndex();
	m_leafChunkCount = 0;
	const uint32_t* islandIDs = header->getFamilyGraph()->getIslandIds();
	uint32_t lastGraphNodeIndex = invalidIndex<uint32_t>();
	uint32_t nextGraphNodeIndex = invalidIndex<uint32_t>();
	bool overflow = false;
	for (uint32_t graphNodeIndex = m_firstGraphNodeIndex; !isInvalidIndex(graphNodeIndex); graphNodeIndex = nextGraphNodeIndex)
	{
		nextGraphNodeIndex = graphNodeIndexLinks[graphNodeIndex];
		const uint32_t islandID = islandIDs[graphNodeIndex];

		if (islandID == thisActorIndex)
		{
			m_leafChunkCount += subtreeLeafChunkCounts[graphChunkIndices[graphNodeIndex]];
			lastGraphNodeIndex = graphNodeIndex;
			continue;	// Leave the chunk in this actor
		}

		// Remove link from this actor
		if (isInvalidIndex(lastGraphNodeIndex))
		{
			m_firstGraphNodeIndex = nextGraphNodeIndex;
		}
		else
		{
			graphNodeIndexLinks[lastGraphNodeIndex] = nextGraphNodeIndex;
		}
		graphNodeIndexLinks[graphNodeIndex] = invalidIndex<uint32_t>();
		--m_graphNodeCount;

		// See if the chunk had been removed
		if (islandID == invalidIndex<uint32_t>())
		{
			continue;
		}

		// Get new actor if the islandID is valid
		Actor* newActor = header->borrowActor(islandID);

		// Check new actor to see if we're adding the first chunk
		if (isInvalidIndex(newActor->m_firstGraphNodeIndex))
		{
			// See if we can fit it in the output list
			if (newActorCount < newActorsSize)
			{
				newActors[newActorCount++] = newActor;
			}
			else
			{
				overflow = true;
			}
		}

		// Put link in new actor
		graphNodeIndexLinks[graphNodeIndex] = newActor->m_firstGraphNodeIndex;
		newActor->m_firstGraphNodeIndex = graphNodeIndex;
		++newActor->m_graphNodeCount;
		// Add to the actor's leaf chunk count
		newActor->m_leafChunkCount += subtreeLeafChunkCounts[graphChunkIndices[graphNodeIndex]];
	}

	if (m_graphNodeCount > 0)
	{
		// There are still chunks in this actor.  See if we can fit this in the output list.
		if (newActorCount < newActorsSize)
		{
			newActors[newActorCount++] = this;
		}
		else
		{
			overflow = true;
		}
	}
	else
	{
		// No more chunks; release this actor.
		release();
	}

	if (overflow)
	{
		NVBLAST_LOG_WARNING(logFn, "Nv::Blast::Actor::partitionMultipleGraphNodes: input newActors array could not hold all actors generated.");
	}

	return newActorCount;
}


uint32_t Actor::partitionSingleLowerSupportChunk(Actor** newActors, uint32_t newActorsSize, NvBlastLog logFn)
{
	NVBLAST_ASSERT(newActorsSize == 0 || newActors != nullptr);

	// Ensure this is a single subsupport chunk, no partitioning
	if (m_graphNodeCount > 1)
	{
		NVBLAST_LOG_WARNING(logFn, "Nv::Blast::Actor::partitionSingleLowerSupportChunk: actor is not a single lower-support chunk, and cannot be partitioned by this function.");
		return 0;
	}

	FamilyHeader* header = getFamilyHeader();

	// The conditional (visible vs. support chunk) is needed because we allow single-child chunk chains
	// This makes it possible that an actor with a single support chunk will have a different visible chunk (ancestor of the support chunk)
	const uint32_t chunkIndex = m_graphNodeCount == 0 ? m_firstVisibleChunkIndex : getGraph()->getChunkIndices()[m_firstGraphNodeIndex];
	NVBLAST_ASSERT(isInvalidIndex(header->getVisibleChunkIndexLinks()[chunkIndex].m_adj[1]));

	const NvBlastChunk& chunk = header->m_asset->getChunks()[chunkIndex];
	uint32_t childCount = chunk.childIndexStop - chunk.firstChildIndex;

	// Warn if we cannot fit all child chunks in the output list
	if (childCount > newActorsSize)
	{
		NVBLAST_LOG_WARNING(logFn, "Nv::Blast::Actor::partitionSingleLowerSupportChunk: input newActors array will not hold all actors generated.");
		childCount = newActorsSize;
	}

	// Return if no chunks will be created.
	if (childCount == 0)
	{
		return 0;
	}

	// Activate a new actor for every child chunk
	const Asset* asset = getAsset();
	const NvBlastChunk* chunks = asset->getChunks();
	const uint32_t firstChildIndex = chunks[chunkIndex].firstChildIndex;
	for (uint32_t i = 0; i < childCount; ++i)
	{
		const uint32_t childIndex = firstChildIndex + i;
		NVBLAST_ASSERT(childIndex >= asset->m_firstSubsupportChunkIndex);
		const uint32_t actorIndex = asset->m_graph.m_nodeCount + (childIndex - asset->m_firstSubsupportChunkIndex);
		NVBLAST_ASSERT(!header->isActorActive(actorIndex));
		newActors[i] = header->borrowActor(actorIndex);
		newActors[i]->m_firstVisibleChunkIndex = childIndex;
		newActors[i]->m_visibleChunkCount = 1;
		newActors[i]->m_leafChunkCount = asset->getSubtreeLeafChunkCounts()[childIndex];
	}

	// Release this actor
	release();

	return childCount;
}


void Actor::updateVisibleChunksFromGraphNodes()
{
	// Only apply this to upper-support chunk actors
	if (m_graphNodeCount == 0)
	{
		return;
	}

	const Asset* asset = getAsset();

	const uint32_t thisActorIndex = getIndex();

	// Get various arrays
	FamilyHeader* header = getFamilyHeader();
	Actor* actors = header->getActors();
	IndexDLink<uint32_t>* visibleChunkIndexLinks = header->getVisibleChunkIndexLinks();
	uint32_t* chunkActorIndices = header->getChunkActorIndices();
	const Nv::Blast::SupportGraph& graph = asset->m_graph;
	const uint32_t* graphChunkIndices = graph.getChunkIndices();
	const NvBlastChunk* chunks = asset->getChunks();
	const uint32_t upperSupportChunkCount = asset->getUpperSupportChunkCount();

	// Iterate over all graph nodes and update visible chunk list
	const uint32_t* graphNodeIndexLinks = header->getGraphNodeIndexLinks();
	for (uint32_t graphNodeIndex = m_firstGraphNodeIndex; !isInvalidIndex(graphNodeIndex); graphNodeIndex = graphNodeIndexLinks[graphNodeIndex])
	{
		updateVisibleChunksFromSupportChunk<Actor>(actors, visibleChunkIndexLinks, chunkActorIndices, thisActorIndex, graphChunkIndices[graphNodeIndex], chunks, upperSupportChunkCount);
	}
}

} // namespace Blast
} // namespace Nv


// API implementation

extern "C"
{

NvBlastActor* NvBlastFamilyCreateFirstActor(NvBlastFamily* family, const NvBlastActorDesc* desc, void* scratch, NvBlastLog logFn)
{
	NVBLAST_CHECK(family != nullptr, logFn, "NvBlastFamilyCreateFirstActor: NULL family input.", return nullptr);
	NVBLAST_CHECK(desc != nullptr, logFn, "NvBlastFamilyCreateFirstActor: NULL desc input.", return nullptr);
	NVBLAST_CHECK(scratch != nullptr, logFn, "NvBlastFamilyCreateFirstActor: NULL scratch input.", return nullptr);

	return Nv::Blast::Actor::create(family, desc, scratch, logFn);
}


size_t NvBlastFamilyGetRequiredScratchForCreateFirstActor(const NvBlastFamily* family, NvBlastLog logFn)
{
	NVBLAST_CHECK(family != nullptr, logFn, "NvBlastFamilyGetRequiredScratchForCreateFirstActor: NULL family input.", return 0);
	NVBLAST_CHECK(reinterpret_cast<const Nv::Blast::FamilyHeader*>(family)->m_asset != nullptr, 
		logFn, "NvBlastFamilyGetRequiredScratchForCreateFirstActor: family has NULL asset.", return 0);

	return Nv::Blast::Actor::createRequiredScratch(family);
}


bool NvBlastActorDeactivate(NvBlastActor* actor, NvBlastLog logFn)
{
	NVBLAST_CHECK(actor != nullptr, logFn, "NvBlastActorDeactivate: NULL actor input.", return false);

	Nv::Blast::Actor& a = *static_cast<Nv::Blast::Actor*>(actor);

	if (!a.isActive())
	{
		NVBLAST_LOG_WARNING(logFn, "NvBlastActorDeactivate: inactive actor input.");
	}

	return a.release();
}


uint32_t NvBlastActorGetVisibleChunkCount(const NvBlastActor* actor, NvBlastLog logFn)
{
	NVBLAST_CHECK(actor != nullptr, logFn, "NvBlastActorGetVisibleChunkCount: NULL actor input.", return 0);

	const Nv::Blast::Actor& a = *static_cast<const Nv::Blast::Actor*>(actor);

	if (!a.isActive())
	{
		NVBLAST_LOG_ERROR(logFn, "NvBlastActorGetVisibleChunkCount: inactive actor input.");
		return 0;
	}

	return a.getVisibleChunkCount();
}


uint32_t NvBlastActorGetVisibleChunkIndices(uint32_t* visibleChunkIndices, uint32_t visibleChunkIndicesSize, const NvBlastActor* actor, NvBlastLog logFn)
{
	NVBLAST_CHECK(visibleChunkIndices != nullptr, logFn, "NvBlastActorGetVisibleChunkIndices: NULL visibleChunkIndices pointer input.",	return 0);
	NVBLAST_CHECK(actor != nullptr, logFn, "NvBlastActorGetVisibleChunkIndices: NULL actor pointer input.", return 0);

	const Nv::Blast::Actor& a = *static_cast<const Nv::Blast::Actor*>(actor);

	if (!a.isActive())
	{
		NVBLAST_LOG_ERROR(logFn, "NvBlastActorGetVisibleChunkIndices: inactive actor pointer input.");
		return 0;
	}

	// Iterate through visible chunk list and write to supplied array
	uint32_t indexCount = 0;
	for (Nv::Blast::Actor::VisibleChunkIt i = a; indexCount < visibleChunkIndicesSize && (bool)i; ++i)
	{
		visibleChunkIndices[indexCount++] = (uint32_t)i;
	}

	return indexCount;
}


uint32_t NvBlastActorGetGraphNodeCount(const NvBlastActor* actor, NvBlastLog logFn)
{
	NVBLAST_CHECK(actor != nullptr, logFn, "NvBlastActorGetGraphNodeCount: NULL actor pointer input.", return 0);

	const Nv::Blast::Actor& a = *static_cast<const Nv::Blast::Actor*>(actor);

	if (!a.isActive())
	{
		NVBLAST_LOG_ERROR(logFn, "NvBlastActorGetGraphNodeCount: inactive actor pointer input.");
		return 0;
	}

	return a.getGraphNodeCount();
}


uint32_t NvBlastActorGetGraphNodeIndices(uint32_t* graphNodeIndices, uint32_t graphNodeIndicesSize, const NvBlastActor* actor, NvBlastLog logFn)
{
	NVBLAST_CHECK(graphNodeIndices != nullptr, logFn, "NvBlastActorGetGraphNodeIndices: NULL graphNodeIndices pointer input.", return 0);
	NVBLAST_CHECK(actor != nullptr, logFn, "NvBlastActorGetGraphNodeIndices: NULL actor pointer input.", return 0);

	const Nv::Blast::Actor& a = *static_cast<const Nv::Blast::Actor*>(actor);

	if (!a.isActive())
	{
		NVBLAST_LOG_ERROR(logFn, "NvBlastActorGetGraphNodeIndices: inactive actor pointer input.");
		return 0;
	}

	// Iterate through graph node list and write to supplied array
	uint32_t indexCount = 0;
	for (Nv::Blast::Actor::GraphNodeIt i = a; indexCount < graphNodeIndicesSize && (bool)i; ++i)
	{
		graphNodeIndices[indexCount++] = (uint32_t)i;
	}

	return indexCount;
}


const float* NvBlastActorGetBondHealths(const NvBlastActor* actor, NvBlastLog logFn)
{
	NVBLAST_CHECK(actor != nullptr, logFn, "NvBlastActorGetBondHealths: NULL actor pointer input.", return nullptr);

	const Nv::Blast::Actor& a = *static_cast<const Nv::Blast::Actor*>(actor);

	if (!a.isActive())
	{
		NVBLAST_LOG_ERROR(logFn, "NvBlastActorGetBondHealths: inactive actor pointer input.");
		return nullptr;
	}

	return a.getFamilyHeader()->getBondHealths();
}


NvBlastFamily* NvBlastActorGetFamily(const NvBlastActor* actor, NvBlastLog logFn)
{
	NVBLAST_CHECK(actor != nullptr, logFn, "NvBlastActorGetFamily: NULL actor pointer input.", return nullptr);

	const Nv::Blast::Actor& a = *static_cast<const Nv::Blast::Actor*>(actor);

	if (!a.isActive())
	{
		NVBLAST_LOG_ERROR(logFn, "NvBlastActorGetFamily: inactive actor pointer input.");
		return nullptr;
	}

	return reinterpret_cast<NvBlastFamily*>(a.getFamilyHeader());
}


uint32_t NvBlastActorGetIndex(const NvBlastActor* actor, NvBlastLog logFn)
{
	NVBLAST_CHECK(actor != nullptr, logFn, "NvBlastActorGetIndex: NULL actor pointer input.", return Nv::Blast::invalidIndex<uint32_t>());

	const Nv::Blast::Actor& a = *static_cast<const Nv::Blast::Actor*>(actor);

	if (!a.isActive())
	{
		NVBLAST_LOG_ERROR(logFn, "NvBlastActorGetIndex: actor is not active.");
		return Nv::Blast::invalidIndex<uint32_t>();
	}

	return a.getIndex();
}


void NvBlastActorGenerateFracture
(
	NvBlastFractureBuffers* commandBuffers,
	const NvBlastActor* actor,
	const NvBlastDamageProgram program,
	const NvBlastProgramParams* programParams,
	NvBlastLog logFn,
	NvBlastTimers* timers
)
{
	NVBLAST_CHECK(commandBuffers != nullptr, logFn, "NvBlastActorGenerateFracture: NULL commandBuffers pointer input.", return);
	NVBLAST_CHECK(actor != nullptr, logFn, "NvBlastActorGenerateFracture: NULL actor pointer input.", return);

	const Nv::Blast::Actor& a = *static_cast<const Nv::Blast::Actor*>(actor);

	if (!a.isActive())
	{
		NVBLAST_LOG_ERROR(logFn, "NvBlastActorGenerateFracture: actor is not active.");
		commandBuffers->bondFractureCount = 0;
		commandBuffers->chunkFractureCount = 0;
		return;
	}

	a.generateFracture(commandBuffers, program, programParams, logFn, timers);
}


void NvBlastActorApplyFracture
(
	NvBlastFractureBuffers* eventBuffers,
	NvBlastActor* actor,
	const NvBlastFractureBuffers* commands,
	NvBlastLog logFn,
	NvBlastTimers* timers
)
{
	NVBLAST_CHECK(actor != nullptr, logFn, "NvBlastActorApplyFracture: NULL actor pointer input.", return);
	NVBLAST_CHECK(commands != nullptr, logFn, "NvBlastActorApplyFracture: NULL commands pointer input.", return);
	NVBLAST_CHECK(Nv::Blast::isValid(commands), logFn, "NvBlastActorApplyFracture: commands memory is NULL but size is > 0.", return);

	Nv::Blast::Actor& a = *static_cast<Nv::Blast::Actor*>(actor);

	if (!a.isActive())
	{
		NVBLAST_LOG_ERROR(logFn, "NvBlastActorApplyFracture: actor is not active.");
		if (eventBuffers != nullptr)
		{
			eventBuffers->bondFractureCount = 0;
			eventBuffers->chunkFractureCount = 0;
		}
		return;
	}

	a.applyFracture(eventBuffers, commands, logFn, timers);
}


size_t NvBlastActorGetRequiredScratchForSplit(const NvBlastActor* actor, NvBlastLog logFn)
{
	NVBLAST_CHECK(actor != nullptr, logFn, "NvBlastActorGetRequiredScratchForSplit: NULL actor input.", return 0);

	const Nv::Blast::Actor& a = *static_cast<const Nv::Blast::Actor*>(actor);

	if (!a.isActive())
	{
		NVBLAST_LOG_ERROR(logFn, "NvBlastActorGetRequiredScratchForSplit: actor is not active.");
		return 0;
	}

	return a.splitRequiredScratch();
}


uint32_t NvBlastActorGetMaxActorCountForSplit(const NvBlastActor* actor, NvBlastLog logFn)
{
	NVBLAST_CHECK(actor != nullptr, logFn, "NvBlastActorGetMaxActorCountForSplit: NULL actor input.", return 0);

	const Nv::Blast::Actor& a = *static_cast<const Nv::Blast::Actor*>(actor);

	if (!a.isActive())
	{
		NVBLAST_LOG_ERROR(logFn, "NvBlastActorGetMaxActorCountForSplit: actor is not active.");
		return 0;
	}

	return a.getLeafChunkCount() + 1; // GWD-167 workaround (+1)
}


uint32_t NvBlastActorSplit
(
	NvBlastActorSplitEvent* result,
	NvBlastActor* actor,
	uint32_t newActorsMaxCount,
	void* scratch,
	NvBlastLog logFn,
	NvBlastTimers* timers
)
{
	NVBLAST_CHECK(result != nullptr, logFn, "NvBlastActorSplit: NULL result pointer input.", return 0);
	NVBLAST_CHECK(newActorsMaxCount > 0 && result->newActors != nullptr, logFn, "NvBlastActorSplit: no space for results provided.", return 0);
	NVBLAST_CHECK(actor != nullptr, logFn, "NvBlastActorSplit: NULL actor pointer input.", return 0);
	NVBLAST_CHECK(scratch != nullptr, logFn, "NvBlastActorSplit: NULL scratch pointer input.", return 0);

	Nv::Blast::Actor& a = *static_cast<Nv::Blast::Actor*>(actor);

	if (!a.isActive())
	{
		NVBLAST_LOG_ERROR(logFn, "NvBlastActorGetIndex: actor is not active.");
		return 0;
	}

	return a.split(result, newActorsMaxCount, scratch, logFn, timers);
}


bool NvBlastActorCanFracture(const NvBlastActor* actor, NvBlastLog logFn)
{
	NVBLAST_CHECK(actor != nullptr, logFn, "NvBlastActorCanFracture: NULL actor input.", return false);

	const Nv::Blast::Actor& a = *static_cast<const Nv::Blast::Actor*>(actor);

	if (!a.isActive())
	{
		NVBLAST_LOG_ERROR(logFn, "NvBlastActorCanFracture: actor is not active.");
		return false;
	}

	bool canFracture = true;

	uint32_t graphNodeCount = a.getGraphNodeCount();
	if (graphNodeCount < 2)
	{
		uint32_t chunkHealthIndex = graphNodeCount == 0 ?
			a.getFirstVisibleChunkIndex() - a.getFirstSubsupportChunkIndex() + a.getGraph()->m_nodeCount :
			a.getFirstGraphNodeIndex();
		canFracture = (a.getLowerSupportChunkHealths()[chunkHealthIndex] > 0.0f);
	}

	return canFracture;
}


} // extern "C"


// deprecated API, still used in tests
uint32_t NvBlastActorClosestChunk(const float point[4], const NvBlastActor* actor, NvBlastLog logFn)
{
	const Nv::Blast::Actor& a = *static_cast<const Nv::Blast::Actor*>(actor);

	if (a.isSubSupportChunk())
	{
		NVBLAST_LOG_WARNING(logFn, "NvBlastActorClosestChunk: not a graph actor.");
		return Nv::Blast::invalidIndex<uint32_t>();
	}

	uint32_t closestNode = Nv::Blast::findNodeByPositionLinked(
		point,
		a.getFirstGraphNodeIndex(),
		a.getFamilyHeader()->getGraphNodeIndexLinks(),
		a.getAsset()->m_graph.getAdjacencyPartition(),
		a.getAsset()->m_graph.getAdjacentNodeIndices(),
		a.getAsset()->m_graph.getAdjacentBondIndices(),
		a.getAsset()->getBonds(),
		a.getFamilyHeader()->getBondHealths()
		);

	return a.getAsset()->m_graph.getChunkIndices()[closestNode];
}
