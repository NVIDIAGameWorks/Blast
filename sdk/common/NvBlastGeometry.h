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


#ifndef NVBLASTGEOMETRY_H
#define NVBLASTGEOMETRY_H

#include "NvBlastTypes.h"
#include "NvBlastMath.h"
#include "NvBlastAssert.h"

#include <limits>


namespace Nv {
namespace Blast{


/**
Find the closest node to point in the graph. Uses primarily distance to chunk centroids.
Bond normals are expected to be directed from the lower to higher node index.
Cannot be used for graph actors with only the world chunk in the graph.

\param[in]	point						the point to test against
\param[in]	firstGraphNodeIndex			the entry point for familyGraphNodeIndexLinks
\param[in]	familyGraphNodeIndexLinks	the list index links of the actor's graph
\param[in]	adjacencyPartition			the actor's SupportGraph adjacency partition
\param[in]	adjacentNodeIndices			the actor's SupportGraph adjacent node indices
\param[in]	adjacentBondIndices			the actor's SupportGraph adjacent bond indices
\param[in]	assetBonds					the actor's asset bonds
\param[in]	bondHealths					the actor's bond healths
\param[in]	assetChunks					the actor's asset chunks
\param[in]	supportChunkHealths			the actor's graph chunks healths
\param[in]	chunkIndices				maps node index to chunk index in SupportGraph

\return		the index of the node closest to point
*/
NV_FORCE_INLINE	uint32_t findClosestNode(const float point[4], 
	const uint32_t firstGraphNodeIndex, const uint32_t* familyGraphNodeIndexLinks,
	const uint32_t* adjacencyPartition, const uint32_t* adjacentNodeIndices, const uint32_t* adjacentBondIndices,
	const NvBlastBond* assetBonds, const float* bondHealths,
	const NvBlastChunk* assetChunks, const float* supportChunkHealths, const uint32_t* chunkIndices)
{
	// firstGraphNodeIndex could still be the world chunk, however
	// there should be no way a single-node actor that is just the world chunk exists.
	uint32_t nodeIndex = firstGraphNodeIndex;
	// Since there should always be a regular chunk in the graph, it is possible to initialize closestNode
	// as world chunk index but it would always evaluate to some meaningful node index eventually.
	uint32_t closestNode = nodeIndex;
	float minDist = std::numeric_limits<float>().max();

	// find the closest healthy chunk in the graph by its centroid to point distance
	while (!Nv::Blast::isInvalidIndex(nodeIndex))
	{
		if (supportChunkHealths[nodeIndex] > 0.0f)
		{
			uint32_t chunkIndex = chunkIndices[nodeIndex];
			if (!isInvalidIndex(chunkIndex)) // Invalid if this is the world chunk
			{
				const NvBlastChunk& chunk = assetChunks[chunkIndex];
				const float* centroid = chunk.centroid;

				float d[3]; VecMath::sub(point, centroid, d);
				float dist = VecMath::dot(d, d);

				if (dist < minDist)
				{
					minDist = dist;
					closestNode = nodeIndex;
				}
			}
		}
		nodeIndex = familyGraphNodeIndexLinks[nodeIndex];
	}

	// as long as the world chunk is not input as a single-node graph actor
	NVBLAST_ASSERT(!isInvalidIndex(chunkIndices[closestNode]));

	bool iterateOnBonds = true;
	if (iterateOnBonds)
	{
		// improve geometric accuracy by looking on which side of the closest bond the point lies
		// expects bond normals to point from the smaller to the larger node index

		nodeIndex = closestNode;
		minDist = std::numeric_limits<float>().max();

		const uint32_t startIndex = adjacencyPartition[nodeIndex];
		const uint32_t stopIndex = adjacencyPartition[nodeIndex + 1];

		for (uint32_t adjacentIndex = startIndex; adjacentIndex < stopIndex; adjacentIndex++)
		{
			const uint32_t neighbourIndex = adjacentNodeIndices[adjacentIndex];
			const uint32_t neighbourChunk = chunkIndices[neighbourIndex];
			if (!isInvalidIndex(neighbourChunk)) // Invalid if neighbor is the world chunk
			{
				const uint32_t bondIndex = adjacentBondIndices[adjacentIndex];
				// do not follow broken bonds, since it means that neighbor is not actually connected in the graph
				if (bondHealths[bondIndex] > 0.0f && supportChunkHealths[neighbourIndex] > 0.0f)
				{
					const NvBlastBond& bond = assetBonds[bondIndex];

					const float* centroid = bond.centroid;
					float d[3]; VecMath::sub(point, centroid, d);
					float dist = VecMath::dot(d, d);

					if (dist < minDist)
					{
						minDist = dist;
						float s = VecMath::dot(d, bond.normal);
						if (nodeIndex < neighbourIndex)
						{
							closestNode = s < 0.0f ? nodeIndex : neighbourIndex;
						}
						else
						{
							closestNode = s < 0.0f ? neighbourIndex : nodeIndex;
						}
					}
				}
			}
		}
	}

	return closestNode;
}


/**
Find the closest node to point in the graph. Uses primarily distance to bond centroids.
Slower compared to chunk based lookup but may yield better accuracy in some cases.
Bond normals are expected to be directed from the lower to higher node index.
Cannot be used for graph actors with only the world chunk in the graph.

\param[in]	point						the point to test against
\param[in]	firstGraphNodeIndex			the entry point for familyGraphNodeIndexLinks
\param[in]	familyGraphNodeIndexLinks	the list index links of the actor's graph
\param[in]	adjacencyPartition			the actor's SupportGraph adjacency partition
\param[in]	adjacentNodeIndices			the actor's SupportGraph adjacent node indices
\param[in]	adjacentBondIndices			the actor's SupportGraph adjacent bond indices
\param[in]	assetBonds					the actor's asset bonds
\param[in]	bondHealths					the actor's bond healths
\param[in]	chunkIndices				maps node index to chunk index in SupportGraph

\return		the index of the node closest to point
*/
NV_FORCE_INLINE uint32_t findClosestNode(const float point[4],
	const uint32_t firstGraphNodeIndex, const uint32_t* familyGraphNodeIndexLinks,
	const uint32_t* adjacencyPartition, const uint32_t* adjacentNodeIndices, const uint32_t* adjacentBondIndices,
	const NvBlastBond* bonds, const float* bondHealths, const uint32_t* chunkIndices)
{
	// firstGraphNodeIndex could still be the world chunk, however
	// there should be no way a single-node actor that is just the world chunk exists.
	uint32_t nodeIndex = firstGraphNodeIndex;
	// Since there should always be a regular chunk in the graph, it is possible to initialize closestNode
	// as world chunk index but it would always evaluate to some meaningful node index eventually.
	uint32_t closestNode = nodeIndex;
	float minDist = std::numeric_limits<float>().max();

	while (!Nv::Blast::isInvalidIndex(nodeIndex))
	{
		const uint32_t startIndex = adjacencyPartition[nodeIndex];
		const uint32_t stopIndex = adjacencyPartition[nodeIndex + 1];

		for (uint32_t adjacentIndex = startIndex; adjacentIndex < stopIndex; adjacentIndex++)
		{
			const uint32_t neighbourIndex = adjacentNodeIndices[adjacentIndex];
			if (nodeIndex < neighbourIndex)
			{
				const uint32_t bondIndex = adjacentBondIndices[adjacentIndex];
				if (bondHealths[bondIndex] > 0.0f)
				{
					const NvBlastBond& bond = bonds[bondIndex];

					const float* centroid = bond.centroid;
					float d[3]; VecMath::sub(point, centroid, d);
					float dist = VecMath::dot(d, d);

					if (dist < minDist)
					{
						minDist = dist;
						// if any of the nodes is the world chunk, use the valid one instead
						if (isInvalidIndex(chunkIndices[neighbourIndex]))
						{
							closestNode = nodeIndex;
						}
						else if (isInvalidIndex(chunkIndices[nodeIndex]))
						{
							closestNode = neighbourIndex;
						}
						else
						{
							float s = VecMath::dot(d, bond.normal);
							closestNode = s < 0 ? nodeIndex : neighbourIndex;
						}
					}
				}
			}
		}
		nodeIndex = familyGraphNodeIndexLinks[nodeIndex];
	}

	// as long as the world chunk is not input as a single-node graph actor
	NVBLAST_ASSERT(!isInvalidIndex(chunkIndices[closestNode]));
	return closestNode;
}


} // namespace Blast
} // namespace Nv


#endif // NVBLASTGEOMETRY_H
