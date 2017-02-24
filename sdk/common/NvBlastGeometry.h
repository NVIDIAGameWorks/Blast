/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/


#ifndef NVBLASTGEOMETRY_H
#define NVBLASTGEOMETRY_H

#include "NvBlastTypes.h"
#include "NvBlastMath.h"

#include<limits>


namespace Nv {
namespace Blast{

NV_FORCE_INLINE uint32_t findNodeByPositionLinked(const float point[4],
	const uint32_t firstGraphNodeIndex, const uint32_t* familyGraphNodeIndexLinks,
	const uint32_t* adjacencyPartition, const uint32_t* adjacentNodeIndices, const uint32_t* adjacentBondIndices,
	const NvBlastBond* bonds, const float* bondHealths)
{
	uint32_t nodeIndex = firstGraphNodeIndex;
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
						float s = VecMath::dot(d, bond.normal);
						closestNode = s < 0 ? nodeIndex : neighbourIndex;
					}
				}
			}
		}
		nodeIndex = familyGraphNodeIndexLinks[nodeIndex];
	}

	return closestNode;
}


NV_FORCE_INLINE uint32_t findNodeByPosition(const float point[4],
	const uint32_t graphNodesCount, const uint32_t* graphNodeIndices,
	const uint32_t* adjacencyPartition, const uint32_t* adjacentNodeIndices, const uint32_t* adjacentBondIndices,
	const NvBlastBond* bonds, const float* bondHealths)
{
	uint32_t closestNode = graphNodesCount > 2 ? invalidIndex<uint32_t>() : graphNodeIndices[0];
	float minDist = std::numeric_limits<float>().max();

	for (uint32_t i = 0; i < graphNodesCount; i++)
	{
		const uint32_t nodeIndex = graphNodeIndices[i];
		const uint32_t startIndex = adjacencyPartition[nodeIndex];
		const uint32_t stopIndex = adjacencyPartition[nodeIndex + 1];

		for (uint32_t adjacentIndex = startIndex; adjacentIndex < stopIndex; adjacentIndex++)
		{
			const uint32_t bondIndex = adjacentBondIndices[adjacentIndex];
			if (bondHealths[bondIndex] > 0.0f)
			{
				const uint32_t neighbourIndex = adjacentNodeIndices[adjacentIndex];
				if (nodeIndex < neighbourIndex)
				{
					const NvBlastBond& bond = bonds[bondIndex];

					const float* centroid = bond.centroid;
					float d[3]; VecMath::sub(point, centroid, d);
					float dist = VecMath::dot(d, d);

					if (dist < minDist)
					{
						minDist = dist;
						float s = VecMath::dot(d, bond.normal);
						closestNode = s < 0 ? nodeIndex : neighbourIndex;
					}
				}
			}
		}
	}
	return closestNode;
}


NV_FORCE_INLINE uint32_t findNodeByPosition(const float point[4],
	const uint32_t graphNodesCount, const uint32_t* graphNodeIndices,
	const NvBlastSupportGraph& graph,
	const NvBlastBond* bonds, const float* bondHealths)
{
	return findNodeByPosition(point, graphNodesCount, graphNodeIndices, graph.adjacencyPartition, graph.adjacentNodeIndices, graph.adjacentBondIndices, bonds, bondHealths);
}

} // namespace Blast
} // namespace Nv


#endif // NVBLASTGEOMETRY_H
