#include <algorithm>
#include "gtest/gtest.h"

//#include "NvBlast.h"
#include "NvBlastActor.h"
#include "NvBlastIndexFns.h"

#include "AlignedAllocator.h"

#include "TestAssets.h"
#include "NvBlastActor.h"

static void messageLog(int type, const char* msg, const char* file, int line)
{
	{
		switch (type)
		{
		case NvBlastMessage::Error:	std::cout << "NvBlast Error message in " << file << "(" << line << "): " << msg << "\n";	break;
		case NvBlastMessage::Warning:	std::cout << "NvBlast Warning message in " << file << "(" << line << "): " << msg << "\n";	break;
		case NvBlastMessage::Info:	std::cout << "NvBlast Info message in " << file << "(" << line << "): " << msg << "\n";		break;
		case NvBlastMessage::Debug:	std::cout << "NvBlast Debug message in " << file << "(" << line << "): " << msg << "\n";	break;
		}
	}
}

TEST(CoreTests, IndexStartLookup)
{
	uint32_t lookup[32];
	uint32_t indices[] = {1,1,2,2,4,4,4};

	Nv::Blast::createIndexStartLookup<uint32_t>(lookup, 0, 30, indices, 7, 4);

	EXPECT_EQ(lookup[0], 0);
	EXPECT_EQ(lookup[1], 0);
	EXPECT_EQ(lookup[2], 2);
	EXPECT_EQ(lookup[3], 4);
	EXPECT_EQ(lookup[4], 4);
	EXPECT_EQ(lookup[5], 7);
	EXPECT_EQ(lookup[31], 7);
}

#include "NvBlastGeometry.h"

TEST(CoreTests, FindChunkByPosition)
{
	std::vector<char> scratch;
	const NvBlastAssetDesc& desc = g_assetDescs[0]; // 1-cube
	scratch.resize((size_t)NvBlastGetRequiredScratchForCreateAsset(&desc, nullptr));
	void* amem = alignedAlloc<malloc>(NvBlastGetAssetMemorySize(&desc, nullptr));
	NvBlastAsset* asset = NvBlastCreateAsset(amem, &desc, &scratch[0], nullptr);
	ASSERT_TRUE(asset != nullptr);

	uint32_t expectedNode[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
	const float positions[] = {
		-2.0f, -2.0f, -2.0f,
		+2.0f, -2.0f, -2.0f,
		-2.0f, +2.0f, -2.0f,
		+2.0f, +2.0f, -2.0f,
		-2.0f, -2.0f, +2.0f,
		+2.0f, -2.0f, +2.0f,
		-2.0f, +2.0f, +2.0f,
		+2.0f, +2.0f, +2.0f,
	};
	const float* pos = &positions[0];

	NvBlastActorDesc actorDesc;
	actorDesc.initialBondHealths = actorDesc.initialSupportChunkHealths = nullptr;
	actorDesc.uniformInitialBondHealth = actorDesc.uniformInitialLowerSupportChunkHealth = 1.0f;
	void* fmem = alignedAlloc<malloc>(NvBlastAssetGetFamilyMemorySize(asset, nullptr));
	NvBlastFamily* family = NvBlastAssetCreateFamily(fmem, asset, nullptr);
	scratch.resize((size_t)NvBlastFamilyGetRequiredScratchForCreateFirstActor(family, nullptr));
	NvBlastActor* actor = NvBlastFamilyCreateFirstActor(family, &actorDesc, &scratch[0], nullptr);
	ASSERT_TRUE(actor != nullptr);

	std::vector<uint32_t> graphNodeIndices;
	graphNodeIndices.resize(NvBlastActorGetGraphNodeCount(actor, nullptr));
	const float* bondHealths = NvBlastActorGetBondHealths(actor, messageLog);
	uint32_t graphNodesCount = NvBlastActorGetGraphNodeIndices(graphNodeIndices.data(), (uint32_t)graphNodeIndices.size(), actor, nullptr);

	const NvBlastBond* bonds = NvBlastAssetGetBonds(asset, nullptr);
	NvBlastSupportGraph graph = NvBlastAssetGetSupportGraph(asset, nullptr);
	for (int i = 0; i < 8; ++i, pos += 3)
	{
		EXPECT_EQ(expectedNode[i], Nv::Blast::findNodeByPosition(pos, graphNodesCount, graphNodeIndices.data(), graph, bonds, bondHealths));
		EXPECT_EQ(expectedNode[i] + 1, NvBlastActorClosestChunk(pos, actor, nullptr));	// Works because (chunk index) = (node index) + 1 in these cases
	}

	EXPECT_TRUE(NvBlastActorDeactivate(actor, nullptr));
	alignedFree<free>(family);
	alignedFree<free>(asset);
}

TEST(CoreTests, FindChunkByPositionUShape)
{
	/*
	considering this graph

	4->5->6
	^
	|
	1->2->3

	and trying to find chunks by some position
	*/
	const NvBlastChunkDesc uchunks[7] =
	{
		// centroid           volume parent idx		flags                         ID
		{ {0.0f, 0.0f, 0.0f}, 0.0f, UINT32_MAX, NvBlastChunkDesc::NoFlags, 0 },
		{ {0.0f, 0.0f, 0.0f}, 0.0f, 0, NvBlastChunkDesc::SupportFlag, 1 },
		{ {0.0f, 0.0f, 0.0f}, 0.0f, 0, NvBlastChunkDesc::SupportFlag, 2 },
		{ {0.0f, 0.0f, 0.0f}, 0.0f, 0, NvBlastChunkDesc::SupportFlag, 3 },
		{ {0.0f, 0.0f, 0.0f}, 0.0f, 0, NvBlastChunkDesc::SupportFlag, 4 },
		{ {0.0f, 0.0f, 0.0f}, 0.0f, 0, NvBlastChunkDesc::SupportFlag, 5 },
		{ {0.0f, 0.0f, 0.0f}, 0.0f, 0, NvBlastChunkDesc::SupportFlag, 6 }
	};

	const NvBlastBondDesc ubonds[5] =
	{
		// chunks      normal                area  centroid              userData
		{ { 2, 1 }, { { 1.0f, 0.0f, 0.0f }, 1.0f, { 2.0f, 1.0f, 0.0f }, 0 } }, // index swap should not matter
		{ { 2, 3 }, { { 1.0f, 0.0f, 0.0f }, 1.0f, { 4.0f, 1.0f, 0.0f }, 0 } },
		{ { 1, 4 }, { { 0.0f, 1.0f, 0.0f }, 1.0f, { 1.0f, 2.0f, 0.0f }, 0 } },
		{ { 4, 5 }, { { 1.0f, 0.0f, 0.0f }, 1.0f, { 2.0f, 3.0f, 0.0f }, 0 } },
		{ { 5, 6 }, { { 1.0f, 0.0f, 0.0f }, 1.0f, { 4.0f, 3.0f, 0.0f }, 0 } },
	};

	const NvBlastAssetDesc desc = { 7, uchunks, 5, ubonds };
	std::vector<char> scratch;
	scratch.resize((size_t)NvBlastGetRequiredScratchForCreateAsset(&desc, messageLog));
	void* amem = alignedAlloc<malloc>(NvBlastGetAssetMemorySize(&desc, messageLog));
	NvBlastAsset* asset = NvBlastCreateAsset(amem, &desc, &scratch[0], messageLog);
	ASSERT_TRUE(asset != nullptr);

	NvBlastActorDesc actorDesc;
	actorDesc.initialBondHealths = actorDesc.initialSupportChunkHealths = nullptr;
	actorDesc.uniformInitialBondHealth = actorDesc.uniformInitialLowerSupportChunkHealth = 1.0f;
	void* fmem = alignedAlloc<malloc>(NvBlastAssetGetFamilyMemorySize(asset, messageLog));
	NvBlastFamily* family = NvBlastAssetCreateFamily(fmem, asset, nullptr);
	scratch.resize((size_t)NvBlastFamilyGetRequiredScratchForCreateFirstActor(family, messageLog));
	NvBlastActor* actor = NvBlastFamilyCreateFirstActor(family, &actorDesc, &scratch[0], nullptr);
	ASSERT_TRUE(actor != nullptr);

	std::vector<uint32_t> graphNodeIndices;
	graphNodeIndices.resize(NvBlastActorGetGraphNodeCount(actor, nullptr));
	uint32_t graphNodesCount = NvBlastActorGetGraphNodeIndices(graphNodeIndices.data(), (uint32_t)graphNodeIndices.size(), actor, nullptr);

	const NvBlastBond* bonds = NvBlastAssetGetBonds(asset, nullptr);
	NvBlastSupportGraph graph = NvBlastAssetGetSupportGraph(asset, nullptr);

	srand(100);
	for (uint32_t i = 0; i < 100000; i++)
	{
		float rx = 20 * (float)(rand() - 1) / RAND_MAX - 10;
		float ry = 20 * (float)(rand() - 1) / RAND_MAX - 10;
		float rz = 0.0f;
		float rpos[] = { rx, ry, rz };

		// open boundaries
		uint32_t col = std::max(0, std::min(2, int(rx / 2)));
		uint32_t row = std::max(0, std::min(1, int(ry / 2)));
		uint32_t expectedNode = col + row * 3;

		//printf("iteration %i: %.1f %.1f %.1f expected: %d\n", i, rpos[0], rpos[1], rpos[2], expectedNode);
		{
			uint32_t returnedNode = Nv::Blast::findNodeByPosition(rpos, graphNodesCount, graphNodeIndices.data(), graph, bonds, NvBlastActorGetBondHealths(actor, messageLog));
			if (expectedNode != returnedNode)
				Nv::Blast::findNodeByPosition(rpos, graphNodesCount, graphNodeIndices.data(), graph, bonds, NvBlastActorGetBondHealths(actor, messageLog));
			EXPECT_EQ(expectedNode, returnedNode);
		}
		{
			// +1 to account for graph vs. asset indices
			uint32_t expectedChunk = expectedNode + 1;
			uint32_t returnedChunk = NvBlastActorClosestChunk(rpos, actor, nullptr);
			if (expectedChunk != returnedChunk)
				NvBlastActorClosestChunk(rpos, actor, nullptr);
			EXPECT_EQ(expectedChunk, returnedChunk);
		}

	}

	EXPECT_TRUE(NvBlastActorDeactivate(actor, messageLog));

	alignedFree<free>(family);
	alignedFree<free>(asset);
}

TEST(CoreTests, FindChunkByPositionLandlocked)
{
	const NvBlastChunkDesc chunks[10] =
	{
		// centroid           volume parent idx		flags                         ID
		{ {0.0f, 0.0f, 0.0f}, 0.0f, UINT32_MAX, NvBlastChunkDesc::NoFlags, 0 },
		{ {0.0f, 0.0f, 0.0f}, 0.0f, 0, NvBlastChunkDesc::SupportFlag, 1 },
		{ {0.0f, 0.0f, 0.0f}, 0.0f, 0, NvBlastChunkDesc::SupportFlag, 2 },
		{ {0.0f, 0.0f, 0.0f}, 0.0f, 0, NvBlastChunkDesc::SupportFlag, 3 },
		{ {0.0f, 0.0f, 0.0f}, 0.0f, 0, NvBlastChunkDesc::SupportFlag, 4 },
		{ {0.0f, 0.0f, 0.0f}, 0.0f, 0, NvBlastChunkDesc::SupportFlag, 5 },
		{ {0.0f, 0.0f, 0.0f}, 0.0f, 0, NvBlastChunkDesc::SupportFlag, 6 },
		{ {0.0f, 0.0f, 0.0f}, 0.0f, 0, NvBlastChunkDesc::SupportFlag, 7 },
		{ {0.0f, 0.0f, 0.0f}, 0.0f, 0, NvBlastChunkDesc::SupportFlag, 8 },
		{ {0.0f, 0.0f, 0.0f}, 0.0f, 0, NvBlastChunkDesc::SupportFlag, 9 },
	};

	const NvBlastBondDesc bonds[12] =
	{
		// chunks      normal                area  centroid              userData
		{ { 1, 2 }, { { 1.0f, 0.0f, 0.0f }, 1.0f, { 2.0f, 1.0f, 0.0f }, 0 } },
		{ { 2, 3 }, { { 1.0f, 0.0f, 0.0f }, 1.0f, { 4.0f, 1.0f, 0.0f }, 0 } },
		{ { 4, 5 }, { { 1.0f, 0.0f, 0.0f }, 1.0f, { 2.0f, 3.0f, 0.0f }, 0 } },
		{ { 5, 6 }, { { 1.0f, 0.0f, 0.0f }, 1.0f, { 4.0f, 3.0f, 0.0f }, 0 } },
		{ { 7, 8 }, { { 1.0f, 0.0f, 0.0f }, 1.0f, { 2.0f, 5.0f, 0.0f }, 0 } },
		{ { 8, 9 }, { { 1.0f, 0.0f, 0.0f }, 1.0f, { 4.0f, 5.0f, 0.0f }, 0 } },
		{ { 1, 4 }, { { 0.0f, 1.0f, 0.0f }, 1.0f, { 1.0f, 2.0f, 0.0f }, 0 } },
		{ { 2, 5 }, { { 0.0f, 1.0f, 0.0f }, 1.0f, { 3.0f, 2.0f, 0.0f }, 0 } },
		{ { 3, 6 }, { { 0.0f, 1.0f, 0.0f }, 1.0f, { 5.0f, 2.0f, 0.0f }, 0 } },
		{ { 4, 7 }, { { 0.0f, 1.0f, 0.0f }, 1.0f, { 1.0f, 4.0f, 0.0f }, 0 } },
		{ { 5, 8 }, { { 0.0f, 1.0f, 0.0f }, 1.0f, { 3.0f, 4.0f, 0.0f }, 0 } },
		{ { 6, 9 }, { { 0.0f, 1.0f, 0.0f }, 1.0f, { 5.0f, 4.0f, 0.0f }, 0 } },
	};

	const NvBlastAssetDesc desc = { 10, chunks, 12, bonds };
	std::vector<char> scratch;
	scratch.resize((size_t)NvBlastGetRequiredScratchForCreateAsset(&desc, messageLog));
	void* amem = alignedAlloc<malloc>(NvBlastGetAssetMemorySize(&desc, messageLog));
	NvBlastAsset* asset = NvBlastCreateAsset(amem, &desc, &scratch[0], messageLog);
	ASSERT_TRUE(asset != nullptr);

	NvBlastActorDesc actorDesc;
	actorDesc.initialBondHealths = actorDesc.initialSupportChunkHealths = nullptr;
	actorDesc.uniformInitialBondHealth = actorDesc.uniformInitialLowerSupportChunkHealth = 1.0f;
	void* fmem = alignedAlloc<malloc>(NvBlastAssetGetFamilyMemorySize(asset, nullptr));
	NvBlastFamily* family = NvBlastAssetCreateFamily(fmem, asset, nullptr);
	scratch.resize((size_t)NvBlastFamilyGetRequiredScratchForCreateFirstActor(family, nullptr));
	NvBlastActor* actor = NvBlastFamilyCreateFirstActor(family, &actorDesc, &scratch[0], nullptr);
	ASSERT_TRUE(actor != nullptr);

	const NvBlastBond* assetBonds = NvBlastAssetGetBonds(asset, nullptr);
	NvBlastSupportGraph graph = NvBlastAssetGetSupportGraph(asset, nullptr);

	float point[4] = { 3.0f, 3.0f, 0.0f };
	EXPECT_EQ(5, NvBlastActorClosestChunk(point, actor, nullptr));
	{
		std::vector<uint32_t> graphNodeIndices;
		graphNodeIndices.resize(NvBlastActorGetGraphNodeCount(actor, nullptr));
		uint32_t graphNodesCount = NvBlastActorGetGraphNodeIndices(graphNodeIndices.data(), (uint32_t)graphNodeIndices.size(), actor, nullptr);

		EXPECT_EQ(4, Nv::Blast::findNodeByPosition(point, graphNodesCount, graphNodeIndices.data(), graph, assetBonds, NvBlastActorGetBondHealths(actor, messageLog)));
	}

	NvBlastChunkFractureData chunkBuffer[1];
	NvBlastFractureBuffers events = { 0, 1, nullptr, chunkBuffer };

	NvBlastChunkFractureData chunkFracture = { 0, 5, 1.0f };
	NvBlastFractureBuffers commands = { 0, 1, nullptr, &chunkFracture };

	NvBlastActorApplyFracture(&events, actor, &commands, messageLog, nullptr);
	EXPECT_EQ(1, events.chunkFractureCount);

	NvBlastActor* newActors[5];
	NvBlastActorSplitEvent splitEvent = { nullptr, newActors };
	scratch.resize((size_t)NvBlastActorGetRequiredScratchForSplit(actor, messageLog));
	size_t newActorsCount = NvBlastActorSplit(&splitEvent, actor, 5, scratch.data(), messageLog, nullptr);

	ASSERT_EQ(actor, newActors[1]);

	EXPECT_NE(5, NvBlastActorClosestChunk(point, actor, nullptr));

	float point2[4] = { 80.0f, 80.0f, 80.0f };
	EXPECT_EQ(5, NvBlastActorClosestChunk(point2, newActors[0], nullptr));

	{
		const float* bondHealths = NvBlastActorGetBondHealths(actor, messageLog);
		std::vector<uint32_t> graphNodeIndices;
		graphNodeIndices.resize(NvBlastActorGetGraphNodeCount(actor, nullptr));
		uint32_t graphNodesCount = NvBlastActorGetGraphNodeIndices(graphNodeIndices.data(), (uint32_t)graphNodeIndices.size(), actor, nullptr);

		EXPECT_NE(4, Nv::Blast::findNodeByPosition(point, graphNodesCount, graphNodeIndices.data(), graph, assetBonds, bondHealths));

		graphNodeIndices.resize(NvBlastActorGetGraphNodeCount(newActors[0], nullptr));
		graphNodesCount = NvBlastActorGetGraphNodeIndices(graphNodeIndices.data(), (uint32_t)graphNodeIndices.size(), newActors[0], nullptr);

		EXPECT_EQ(4, Nv::Blast::findNodeByPosition(point, graphNodesCount, graphNodeIndices.data(), graph, assetBonds, bondHealths));
	}


	for (uint32_t i = 0; i < newActorsCount; ++i)
	{
		EXPECT_TRUE(NvBlastActorDeactivate(newActors[i], nullptr));
	}

	alignedFree<free>(family);
	alignedFree<free>(asset);
}
