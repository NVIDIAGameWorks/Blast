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


#include "BlastBasePerfTest.h"
#include "TestAssets.h"
#include "NvBlastExtDamageShaders.h"
#include <memory>


static void blast
(
	std::set<NvBlastActor*>& actorsToDamage,
	GeneratorAsset* testAsset,
	GeneratorAsset::Vec3 localPos,
	float minRadius, float maxRadius,
	float compressiveDamage,
	NvBlastTimers& timers
)
{
	std::vector<NvBlastChunkFractureData> chunkEvents; /* num lower-support chunks + bonds */
	std::vector<NvBlastBondFractureData> bondEvents; /* num lower-support chunks + bonds */
	chunkEvents.resize(testAsset->solverChunks.size());
	bondEvents.resize(testAsset->solverBonds.size());

	NvBlastExtRadialDamageDesc damage[] = {
		{
			compressiveDamage,
			{ localPos.x, localPos.y, localPos.z },
			minRadius,
			maxRadius
		}							
	};

	NvBlastProgramParams programParams =
	{
		&damage,
		1,
		nullptr
	};

	NvBlastDamageProgram program = {
		NvBlastExtFalloffGraphShader,
		nullptr
	};

	std::vector<char> splitScratch;
	std::vector<NvBlastActor*> newActors(testAsset->solverChunks.size());

	size_t totalNewActorsCount = 0;
	for (std::set<NvBlastActor*>::iterator k = actorsToDamage.begin(); k != actorsToDamage.end();)
	{
		NvBlastActor* actor = *k;

		NvBlastFractureBuffers events = { (uint32_t)bondEvents.size(), (uint32_t)chunkEvents.size(), bondEvents.data(), chunkEvents.data() };

		NvBlastActorGenerateFracture(&events, actor, program, &programParams, nullptr, &timers);
		NvBlastActorApplyFracture(&events, actor, &events, nullptr, &timers);

		bool removeActor = false;

		if (events.bondFractureCount + events.chunkFractureCount > 0)
		{
			splitScratch.resize((size_t)NvBlastActorGetRequiredScratchForSplit(actor, nullptr));
			NvBlastActorSplitEvent result;
			result.deletedActor = nullptr;
			result.newActors = &newActors[totalNewActorsCount];
			const size_t bufferSize = newActors.size() - totalNewActorsCount;
			const size_t newActorsCount = NvBlastActorSplit(&result, actor, (uint32_t)bufferSize, splitScratch.data(), nullptr, &timers);
			totalNewActorsCount += newActorsCount;
			removeActor = newActorsCount > 0;
		}

		if (removeActor)
		{
			k = actorsToDamage.erase(k);
		}
		else
		{
			++k;
		}
	}

	for (size_t i = 0; i < totalNewActorsCount; ++i)
	{
		actorsToDamage.insert(newActors[i]);
	}
}

typedef BlastBasePerfTest<NvBlastMessage::Warning, 1> BlastBasePerfTestStrict;

class PerfTest : public BlastBasePerfTestStrict
{
public:
	void damageLeafSupportActors(const char* testName, uint32_t assetCount, uint32_t familyCount,	uint32_t damageCount)
	{
		const float relativeDamageRadius = 0.2f;
		const float compressiveDamage = 1.0f;
		const uint32_t minChunkCount = 100;
		const uint32_t maxChunkCount = 10000;

		srand(0);

		for (uint32_t assetNum = 0; assetNum < assetCount; ++assetNum)
		{
			CubeAssetGenerator::Settings settings;
			settings.extents = GeneratorAsset::Vec3(1, 1, 1);
			CubeAssetGenerator::DepthInfo depthInfo;
			depthInfo.slicesPerAxis = GeneratorAsset::Vec3(1, 1, 1);
			depthInfo.flag = NvBlastChunkDesc::Flags::NoFlags;
			settings.depths.push_back(depthInfo);
			uint32_t chunkCount = 1;
			while (chunkCount < minChunkCount)
			{
				uint32_t chunkMul;
				do
				{
					depthInfo.slicesPerAxis = GeneratorAsset::Vec3((float)(1 + rand() % 4), (float)(1 + rand() % 4), (float)(1 + rand() % 4));
					chunkMul = (uint32_t)(depthInfo.slicesPerAxis.x * depthInfo.slicesPerAxis.y * depthInfo.slicesPerAxis.z);
				} while (chunkMul == 1);
				if (chunkCount*chunkMul > maxChunkCount)
				{
					break;
				}
				chunkCount *= chunkMul;
				settings.depths.push_back(depthInfo);
				settings.extents = settings.extents * depthInfo.slicesPerAxis;
			}
			settings.depths.back().flag = NvBlastChunkDesc::SupportFlag;	// Leaves are support

			// Make largest direction unit size
			settings.extents = settings.extents * (1.0f / std::max(settings.extents.x, std::max(settings.extents.y, settings.extents.z)));

			// Create asset
			GeneratorAsset testAsset;
			CubeAssetGenerator::generate(testAsset, settings);

			NvBlastAssetDesc desc;
			desc.chunkDescs = testAsset.solverChunks.data();
			desc.chunkCount = (uint32_t)testAsset.solverChunks.size();
			desc.bondDescs = testAsset.solverBonds.data();
			desc.bondCount = (uint32_t)testAsset.solverBonds.size();

			{
				std::vector<char> scratch;
				scratch.resize((size_t)NvBlastGetRequiredScratchForCreateAsset(&desc, messageLog));
				void* mem = alignedZeroedAlloc(NvBlastGetAssetMemorySize(&desc, messageLog));
				NvBlastAsset* asset = NvBlastCreateAsset(mem, &desc, scratch.data(), messageLog);
				EXPECT_TRUE(asset != nullptr);

				// Generate familes
				for (uint32_t familyNum = 0; familyNum < familyCount; ++familyNum)
				{
					// create actor
					NvBlastActorDesc actorDesc;
					actorDesc.initialBondHealths = nullptr;
					actorDesc.uniformInitialBondHealth = 1.0f;
					actorDesc.initialSupportChunkHealths = nullptr;
					actorDesc.uniformInitialLowerSupportChunkHealth = 1.0f;
					void* mem = alignedZeroedAlloc(NvBlastAssetGetFamilyMemorySize(asset, messageLog));
					NvBlastFamily* family = NvBlastAssetCreateFamily(mem, asset, messageLog);
					scratch.resize((size_t)NvBlastFamilyGetRequiredScratchForCreateFirstActor(family, messageLog));
					EXPECT_TRUE(family != nullptr);
					NvBlastActor* actor = NvBlastFamilyCreateFirstActor(family, &actorDesc, scratch.data(), messageLog);
					EXPECT_TRUE(actor != nullptr);

					// Generate damage
					std::set<NvBlastActor*> actors;
					actors.insert(actor);
					for (uint32_t damageNum = 0; damageNum < damageCount; ++damageNum)
					{
						GeneratorAsset::Vec3 localPos = settings.extents*GeneratorAsset::Vec3((float)rand() / RAND_MAX - 0.5f, (float)rand() / RAND_MAX - 0.5f, (float)rand() / RAND_MAX - 0.5f);

						NvBlastTimers timers;
						NvBlastTimersReset(&timers);
						blast(actors, &testAsset, localPos, relativeDamageRadius, relativeDamageRadius*1.2f, compressiveDamage, timers);
						const std::string timingName = std::string(testName) + " asset " + std::to_string(assetNum) + " family " + std::to_string(familyNum) + " damage " + std::to_string(damageNum);
						BlastBasePerfTestStrict::reportData(timingName + " material", timers.material);
						BlastBasePerfTestStrict::reportData(timingName + " fracture", timers.fracture);
						BlastBasePerfTestStrict::reportData(timingName + " island", timers.island);
						BlastBasePerfTestStrict::reportData(timingName + " partition", timers.partition);
						BlastBasePerfTestStrict::reportData(timingName + " visibility", timers.visibility);
					}

					// Release remaining actors
					std::for_each(actors.begin(), actors.end(), [](NvBlastActor* a){ NvBlastActorDeactivate(a, messageLog); });
					actors.clear();

					free(family);
				}

				// Release asset data
				free(asset);
			}
		}
	}
};


// Tests
TEST_F(PerfTest, DamageLeafSupportActorsTestVisibility)
{
	const int trialCount = 1000;
	std::cout << "Trial (of " << trialCount << "): ";
	for (int trial = 1; trial <= trialCount; ++trial)
	{
		if (trial % 100 == 0)
		{
			std::cout << trial << ".. ";
			std::cout.flush();
		}
		damageLeafSupportActors(test_info_->name(), 4, 4, 5);
	}
	std::cout << "done." << std::endl;
}
