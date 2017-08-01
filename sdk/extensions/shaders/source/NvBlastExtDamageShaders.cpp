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
// Copyright (c) 2016-2017 NVIDIA Corporation. All rights reserved.


#include "NvBlastExtDamageShaders.h"
#include "NvBlastIndexFns.h"
#include "NvBlastMath.h"
#include "NvBlastGeometry.h"
#include "NvBlastAssert.h"
#include "NvBlast.h"
#include <cmath> // for abs() on linux

using namespace Nv::Blast;
using namespace Nv::Blast::VecMath;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Profiles
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef float(*ProfileFunction)(float, float, float, float);

float falloffProfile(float min, float max, float x, float f = 1.0f)
{
	if (x > max) return 0.0f;
	if (x < min) return f;

	float y = 1.0f - (x - min) / (max - min);
	return y * f;
}

float cutterProfile(float min, float max, float x, float f = 1.0f)
{
	if (x > max || x < min) return 0.0f;

	return f;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Damage Functions
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef float(*DamageFunction)(const float pos[3], const void* damageDescBuffer, uint32_t damageIndex);

template <ProfileFunction profileFn, typename DescT = NvBlastExtRadialDamageDesc>
float pointDistanceDamage(const float pos[3], const void* damageDescBuffer, uint32_t damageIndex)
{
	const DescT* damageData = reinterpret_cast<const DescT*>(damageDescBuffer);
	const DescT& desc = damageData[damageIndex];

	float relativePosition[3];
	sub(desc.position, pos, relativePosition);
	const float distance = sqrtf(dot(relativePosition, relativePosition));
	const float damage = profileFn(desc.minRadius, desc.maxRadius, distance, desc.damage);
	return damage;
}


// Distance from point 'p' to line segment '(a, b)'
float distanceToSegment(const float p[3], const float a[3], const float b[3])
{
	float v[3];
	sub(b, a, v);

	float w[3];
	sub(p, a, w);

	const float c1 = dot(v, w);
	if (c1 <= 0)
		return length(w);

	const float c2 = dot(v, v);
	if (c2 < c1)
		return dist(p, b);

	const float t = c1 / c2;
	mul(v, t);
	return dist(v, w);
}

template <ProfileFunction profileFn>
float segmentDistanceDamage(const float pos[3], const void* damageDescBuffer, uint32_t damageIndex)
{
	const NvBlastExtSegmentRadialDamageDesc* damageData = reinterpret_cast<const NvBlastExtSegmentRadialDamageDesc*>(damageDescBuffer);
	const NvBlastExtSegmentRadialDamageDesc& desc = damageData[damageIndex];

	const float distance = distanceToSegment(pos, desc.position0, desc.position1);
	const float damage = profileFn(desc.minRadius, desc.maxRadius, distance, desc.damage);
	return damage;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												Radial Graph Shader Template
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <DamageFunction damageFn>
void RadialProfileGraphShader(NvBlastFractureBuffers* commandBuffers, const NvBlastGraphShaderActor* actor, const NvBlastProgramParams* params)
{
	const uint32_t* graphNodeIndexLinks = actor->graphNodeIndexLinks;
	const uint32_t firstGraphNodeIndex = actor->firstGraphNodeIndex;
	const uint32_t*	adjacencyPartition = actor->adjacencyPartition;
	const uint32_t*	adjacentNodeIndices = actor->adjacentNodeIndices;
	const uint32_t*	adjacentBondIndices = actor->adjacentBondIndices;
	const NvBlastBond* assetBonds = actor->assetBonds;
	const float* familyBondHealths = actor->familyBondHealths;

	const uint32_t damageCount = params->damageDescCount;

	uint32_t outCount = 0;

	uint32_t currentNodeIndex = firstGraphNodeIndex;
	while (!Nv::Blast::isInvalidIndex(currentNodeIndex))
	{
		for (uint32_t adj = adjacencyPartition[currentNodeIndex]; adj < adjacencyPartition[currentNodeIndex + 1]; adj++)
		{
			uint32_t adjacentNodeIndex = adjacentNodeIndices[adj];
			if (currentNodeIndex < adjacentNodeIndex)
			{
				uint32_t bondIndex = adjacentBondIndices[adj];

				// skip bonds that are already broken or were visited already
				// TODO: investigate why testing against health > -1.0f seems slower
				// could reuse the island edge bitmap instead
				if ((familyBondHealths[bondIndex] > 0.0f))
				{

					const NvBlastBond& bond = assetBonds[bondIndex];

					float totalBondDamage = 0.0f;

					for (uint32_t damageIndex = 0; damageIndex < damageCount; damageIndex++)
					{
						totalBondDamage += damageFn(bond.centroid, params->damageDescBuffer, damageIndex);
					}

					if (totalBondDamage > 0.0f)
					{
						NvBlastBondFractureData& outCommand = commandBuffers->bondFractures[outCount++];
						outCommand.nodeIndex0 = currentNodeIndex;
						outCommand.nodeIndex1 = adjacentNodeIndex;
						outCommand.health = totalBondDamage;
					}
				}
			}
		}
		currentNodeIndex = graphNodeIndexLinks[currentNodeIndex];
	}

	commandBuffers->bondFractureCount = outCount;
	commandBuffers->chunkFractureCount = 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//											Radial Single Shader Template
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <DamageFunction damageFn>
void RadialProfileSubgraphShader(NvBlastFractureBuffers* commandBuffers, const NvBlastSubgraphShaderActor* actor, const NvBlastProgramParams* params)
{
	uint32_t chunkFractureCount = 0;
	uint32_t chunkFractureCountMax = commandBuffers->chunkFractureCount;
	const uint32_t chunkIndex = actor->chunkIndex;
	const NvBlastChunk* assetChunks = actor->assetChunks;
	const NvBlastChunk& chunk = assetChunks[chunkIndex];

	float totalDamage = 0.0f;
	for (uint32_t damageIndex = 0; damageIndex < params->damageDescCount; ++damageIndex)
	{
		totalDamage += damageFn(chunk.centroid, params->damageDescBuffer, damageIndex);
	}

	if (totalDamage > 0.0f && chunkFractureCount < chunkFractureCountMax)
	{
		NvBlastChunkFractureData& frac = commandBuffers->chunkFractures[chunkFractureCount++];
		frac.chunkIndex = chunkIndex;
		frac.health = totalDamage;
	}

	commandBuffers->bondFractureCount = 0;
	commandBuffers->chunkFractureCount = chunkFractureCount;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												Radial Shaders Instantiation
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void NvBlastExtFalloffGraphShader(NvBlastFractureBuffers* commandBuffers, const NvBlastGraphShaderActor* actor, const NvBlastProgramParams* params)
{
	RadialProfileGraphShader<pointDistanceDamage<falloffProfile>>(commandBuffers, actor, params);
}

void NvBlastExtFalloffSubgraphShader(NvBlastFractureBuffers* commandBuffers, const NvBlastSubgraphShaderActor* actor, const NvBlastProgramParams* params)
{
	RadialProfileSubgraphShader<pointDistanceDamage<falloffProfile>>(commandBuffers, actor, params);
}

void NvBlastExtCutterGraphShader(NvBlastFractureBuffers* commandBuffers, const NvBlastGraphShaderActor* actor, const NvBlastProgramParams* params)
{
	RadialProfileGraphShader<pointDistanceDamage<cutterProfile>>(commandBuffers, actor, params);
}

void NvBlastExtCutterSubgraphShader(NvBlastFractureBuffers* commandBuffers, const NvBlastSubgraphShaderActor* actor, const NvBlastProgramParams* params)
{
	RadialProfileSubgraphShader<pointDistanceDamage<cutterProfile>>(commandBuffers, actor, params);
}

void NvBlastExtSegmentFalloffGraphShader(NvBlastFractureBuffers* commandBuffers, const NvBlastGraphShaderActor* actor, const NvBlastProgramParams* params)
{
	RadialProfileGraphShader<segmentDistanceDamage<falloffProfile>>(commandBuffers, actor, params);
}

void NvBlastExtSegmentFalloffSubgraphShader(NvBlastFractureBuffers* commandBuffers, const NvBlastSubgraphShaderActor* actor, const NvBlastProgramParams* params)
{
	RadialProfileSubgraphShader<segmentDistanceDamage<falloffProfile>>(commandBuffers, actor, params);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Shear Shader
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void NvBlastExtShearGraphShader(NvBlastFractureBuffers* commandBuffers, const NvBlastGraphShaderActor* actor, const NvBlastProgramParams* params)
{
	uint32_t chunkFractureCount = 0;
	uint32_t chunkFractureCountMax = commandBuffers->chunkFractureCount;
	uint32_t bondFractureCount = 0;
	uint32_t bondFractureCountMax = commandBuffers->bondFractureCount;

	for (uint32_t i = 0; i < params->damageDescCount; ++i)
	{
		const NvBlastExtShearDamageDesc& desc = reinterpret_cast<const NvBlastExtShearDamageDesc*>(params->damageDescBuffer)[i];
		const uint32_t* graphNodeIndexLinks = actor->graphNodeIndexLinks;
		const uint32_t firstGraphNodeIndex = actor->firstGraphNodeIndex;
		const uint32_t* chunkIndices = actor->chunkIndices;
		const uint32_t*	adjacencyPartition = actor->adjacencyPartition;
		const uint32_t*	adjacentNodeIndices = actor->adjacentNodeIndices;
		const uint32_t*	adjacentBondIndices = actor->adjacentBondIndices;
		const NvBlastBond* assetBonds = actor->assetBonds;
		const NvBlastChunk* assetChunks = actor->assetChunks;
		const float* familyBondHealths = actor->familyBondHealths;
		const float* supportChunkHealths = actor->supportChunkHealths;

		uint32_t closestNode = findClosestNode(desc.position
			, firstGraphNodeIndex, graphNodeIndexLinks
			, adjacencyPartition, adjacentNodeIndices, adjacentBondIndices
			, assetBonds, familyBondHealths
			, assetChunks, supportChunkHealths, chunkIndices);

		uint32_t nodeIndex = closestNode;
		float maxDist = 0.0f;
		uint32_t nextNode = invalidIndex<uint32_t>();

		if (chunkFractureCount < chunkFractureCountMax)
		{
			const uint32_t chunkIndex = chunkIndices[nodeIndex];
			const NvBlastChunk& chunk = assetChunks[chunkIndex];
			NvBlastChunkFractureData& frac = commandBuffers->chunkFractures[chunkFractureCount++];
			frac.chunkIndex = chunkIndex;
			frac.health = pointDistanceDamage<falloffProfile, NvBlastExtShearDamageDesc>(chunk.centroid, params->damageDescBuffer, i);
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

				float shear = 1 * std::abs(1 - std::abs(VecMath::dot(desc.normal, bond.normal)));

				float d[3]; VecMath::sub(bond.centroid, desc.position, d);
				float ahead = VecMath::dot(d, desc.normal);
				if (ahead > maxDist)
				{
					maxDist = ahead;
					nextNode = neighbourIndex;
				}

				const float damage = pointDistanceDamage<falloffProfile, NvBlastExtShearDamageDesc>(bond.centroid, params->damageDescBuffer, i);
				if (damage > 0.0f && bondFractureCount < bondFractureCountMax)
				{
					NvBlastBondFractureData& frac = commandBuffers->bondFractures[bondFractureCount++];
					frac.userdata = bond.userData;
					frac.nodeIndex0 = nodeIndex;
					frac.nodeIndex1 = neighbourIndex;
					frac.health = shear * damage;
				}
			}

			if (nodeIndex == nextNode)
				break;

			nodeIndex = nextNode;
		} while (!isInvalidIndex(nextNode));
	}

	commandBuffers->bondFractureCount = bondFractureCount;
	commandBuffers->chunkFractureCount = chunkFractureCount;
}

void NvBlastExtShearSubgraphShader(NvBlastFractureBuffers* commandBuffers, const NvBlastSubgraphShaderActor* actor, const NvBlastProgramParams* params)
{
	RadialProfileSubgraphShader<pointDistanceDamage<falloffProfile, NvBlastExtShearDamageDesc>>(commandBuffers, actor, params);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												Helper Functions
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool NvBlastExtDamageActorRadialFalloff(NvBlastActor* actor, NvBlastFractureBuffers* buffers, const NvBlastExtRadialDamageDesc* damageDescBuffer, uint32_t damageDescCount, const NvBlastExtMaterial* material, NvBlastLog logFn, NvBlastTimers* timers)
{
	NvBlastDamageProgram program =
	{
		NvBlastExtFalloffGraphShader,
		NvBlastExtFalloffSubgraphShader
	};

	NvBlastProgramParams params =
	{
		damageDescBuffer,
		damageDescCount,
		material
	};

	NvBlastActorGenerateFracture(buffers, actor, program, &params, logFn, timers);
	if (buffers->bondFractureCount > 0 || buffers->chunkFractureCount > 0)
	{
		NvBlastActorApplyFracture(nullptr, actor, buffers, logFn, timers);
		return true;
	}

	return false;
}