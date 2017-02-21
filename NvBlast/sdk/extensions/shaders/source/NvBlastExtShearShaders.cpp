/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NvBlastExtDamageShaders.h"
#include "NvBlastIndexFns.h"
#include "NvBlastMath.h"
#include "NvBlastGeometry.h"
#include "NvBlastAssert.h"
#include "stdlib.h" // for abs() on linux

using namespace Nv::Blast;
using namespace Nv::Blast::VecMath;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Graph Shader
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void NvBlastExtShearGraphShader(NvBlastFractureBuffers* commandBuffers, const NvBlastGraphShaderActor* actor, const NvBlastProgramParams* params)
{
	const NvBlastExtMaterial* materialProperties = reinterpret_cast<const NvBlastExtMaterial*>(params->material);
	const float graphChunkThreshold = materialProperties->graphChunkThreshold;
	const float bondTangentialThreshold = materialProperties->bondTangentialThreshold;
	const float damageAttenuation = 1.0f - materialProperties->damageAttenuation;

	uint32_t chunkFractureCount = 0;
	uint32_t chunkFractureCountMax = commandBuffers->chunkFractureCount;
	uint32_t bondFractureCount = 0;
	uint32_t bondFractureCountMax = commandBuffers->bondFractureCount;

	for (uint32_t i = 0; i < params->damageDescCount; ++i)
	{
		const NvBlastExtShearDamageDesc& damage = reinterpret_cast<const NvBlastExtShearDamageDesc*>(params->damageDescBuffer)[i];

		const uint32_t* graphNodeIndexLinks = actor->graphNodeIndexLinks;
		const uint32_t firstGraphNodeIndex = actor->firstGraphNodeIndex;
		const uint32_t* chunkIndices = actor->chunkIndices;
		const uint32_t*	adjacencyPartition = actor->adjacencyPartition;
		const uint32_t*	adjacentNodeIndices = actor->adjacentNodeIndices;
		const uint32_t*	adjacentBondIndices = actor->adjacentBondIndices;
		const NvBlastBond* assetBonds = actor->assetBonds;
		const float* familyBondHealths = actor->familyBondHealths;

		uint32_t closestNode = findNodeByPositionLinked(damage.position, firstGraphNodeIndex, graphNodeIndexLinks, adjacencyPartition, adjacentNodeIndices, adjacentBondIndices, assetBonds, familyBondHealths);
		NVBLAST_ASSERT(!isInvalidIndex(closestNode));

		float damageDir[3];
		float damageMag = VecMath::normal(damage.shear, damageDir);

		uint32_t nodeIndex = closestNode;
		float maxDist = 0.0f;
		uint32_t nextNode = invalidIndex<uint32_t>();

		if (damageMag > graphChunkThreshold && chunkFractureCount < chunkFractureCountMax)
		{
			NvBlastChunkFractureData& frac = commandBuffers->chunkFractures[chunkFractureCount++];
			frac.chunkIndex = chunkIndices[nodeIndex];
			frac.health = damageMag * 2;
		}

		do {
			const uint32_t startIndex = adjacencyPartition[nodeIndex];
			const uint32_t stopIndex = adjacencyPartition[nodeIndex + 1];


			for (uint32_t adjacentNodeIndex = startIndex; adjacentNodeIndex < stopIndex; adjacentNodeIndex++)
			{
				const uint32_t neighbourIndex = adjacentNodeIndices[adjacentNodeIndex];
				const uint32_t bondIndex = adjacentBondIndices[adjacentNodeIndex];
				const NvBlastBond& bond = assetBonds[bondIndex];

				if (!(familyBondHealths[bondIndex] > 0.0f))
					continue;

				float shear = 1 * abs(1 - abs(VecMath::dot(damage.shear, bond.normal)));

				float d[3]; VecMath::sub(bond.centroid, damage.position, d);
				float ahead = VecMath::dot(d, damage.shear);
				if (ahead > maxDist)
				{
					maxDist = ahead;
					nextNode = neighbourIndex;
				}

				if (shear > bondTangentialThreshold && bondFractureCount < bondFractureCountMax)
				{
					NvBlastBondFractureData& frac = commandBuffers->bondFractures[bondFractureCount++];
					frac.userdata = bond.userData;
					frac.nodeIndex0 = nodeIndex;
					frac.nodeIndex1 = neighbourIndex;
					frac.health = shear;
				}
			}

			if (nodeIndex == nextNode)
				break;

			nodeIndex = nextNode;

			damageMag *= damageAttenuation;
		} while (!isInvalidIndex(nextNode));
	}

	commandBuffers->bondFractureCount = bondFractureCount;
	commandBuffers->chunkFractureCount = chunkFractureCount;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Single Shader
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void NvBlastExtShearSubgraphShader(NvBlastFractureBuffers* commandBuffers, const NvBlastSubgraphShaderActor* actor, const NvBlastProgramParams* params)
{
	const NvBlastExtMaterial* materialProperties = reinterpret_cast<const NvBlastExtMaterial*>(params->material);

	uint32_t chunkFractureCount = 0;

	float totalDamage = 0.0f;
	for (uint32_t i = 0; i < params->damageDescCount; ++i)
	{
		const NvBlastExtShearDamageDesc& damage = reinterpret_cast<const NvBlastExtShearDamageDesc*>(params->damageDescBuffer)[i];

		float damageDir[3];
		float damageMag = VecMath::normal(damage.shear, damageDir);

		if (damageMag > materialProperties->singleChunkThreshold)
		{
			totalDamage += damageMag * 2;
		}
	}

	if (totalDamage > 0.0f)
	{
		NvBlastChunkFractureData& frac = commandBuffers->chunkFractures[chunkFractureCount++];
		frac.chunkIndex = actor->chunkIndex;
		frac.health = totalDamage;
	}

	commandBuffers->bondFractureCount = 0;
	commandBuffers->chunkFractureCount = chunkFractureCount;
}
