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
#include "NvBlast.h"
#include "stdlib.h" // for abs() on linux

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
//												Radial Graph Shader Template
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <ProfileFunction profile>
void RadialProfileGraphShader(NvBlastFractureBuffers* commandBuffers, const NvBlastGraphShaderActor* actor, const NvBlastProgramParams* params)
{
	const uint32_t* graphNodeIndexLinks = actor->graphNodeIndexLinks;
	const uint32_t firstGraphNodeIndex = actor->firstGraphNodeIndex;
	const uint32_t*	adjacencyPartition = actor->adjacencyPartition;
	const uint32_t*	adjacentNodeIndices = actor->adjacentNodeIndices;
	const uint32_t*	adjacentBondIndices = actor->adjacentBondIndices;
	const NvBlastBond* assetBonds = actor->assetBonds;
	const float* familyBondHealths = actor->familyBondHealths;

	const NvBlastExtRadialDamageDesc* damageData = reinterpret_cast<const NvBlastExtRadialDamageDesc*>(params->damageDescBuffer);
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
						const NvBlastExtRadialDamageDesc& damage = damageData[damageIndex];

						float relativePosition[3];
						sub(damage.position, bond.centroid, relativePosition);
						float distance = sqrtf(dot(relativePosition, relativePosition));

						float dir[3];
						normal(relativePosition, dir);

						totalBondDamage += profile(damage.minRadius, damage.maxRadius, distance, damage.compressive);
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

template <ProfileFunction profile>
void RadialProfileSubgraphShader(NvBlastFractureBuffers* commandBuffers, const NvBlastSubgraphShaderActor* actor, const NvBlastProgramParams* params)
{
	uint32_t chunkFractureCount = 0;
	const uint32_t chunkIndex = actor->chunkIndex;
	const NvBlastChunk* assetChunks = actor->assetChunks;
	const NvBlastChunk& chunk = assetChunks[chunkIndex];

	float totalDamage = 0.0f;
	for (uint32_t i = 0; i < params->damageDescCount; ++i)
	{
		const NvBlastExtRadialDamageDesc& damage = reinterpret_cast<const NvBlastExtRadialDamageDesc*>(params->damageDescBuffer)[i];

		float relativePosition[3];
		sub(damage.position, chunk.centroid, relativePosition);
		float distance = sqrtf(dot(relativePosition, relativePosition));

		totalDamage += profile(damage.minRadius, damage.maxRadius, distance, damage.compressive);
	}

	if (totalDamage > 0.0f)
	{
		NvBlastChunkFractureData& frac = commandBuffers->chunkFractures[chunkFractureCount++];
		frac.chunkIndex = chunkIndex;
		frac.health = totalDamage;
	}

	commandBuffers->bondFractureCount = 0;
	commandBuffers->chunkFractureCount = chunkFractureCount;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												Shader Instantiation
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void NvBlastExtFalloffGraphShader(NvBlastFractureBuffers* commandBuffers, const NvBlastGraphShaderActor* actor, const NvBlastProgramParams* params)
{
	RadialProfileGraphShader<falloffProfile>(commandBuffers, actor, params);
}

void NvBlastExtFalloffSubgraphShader(NvBlastFractureBuffers* commandBuffers, const NvBlastSubgraphShaderActor* actor, const NvBlastProgramParams* params)
{
	RadialProfileSubgraphShader<falloffProfile>(commandBuffers, actor, params);
}

void NvBlastExtCutterGraphShader(NvBlastFractureBuffers* commandBuffers, const NvBlastGraphShaderActor* actor, const NvBlastProgramParams* params)
{
	RadialProfileGraphShader<cutterProfile>(commandBuffers, actor, params);
}

void NvBlastExtCutterSubgraphShader(NvBlastFractureBuffers* commandBuffers, const NvBlastSubgraphShaderActor* actor, const NvBlastProgramParams* params)
{
	RadialProfileSubgraphShader<cutterProfile>(commandBuffers, actor, params);
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