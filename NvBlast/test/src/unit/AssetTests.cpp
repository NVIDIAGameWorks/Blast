#include "NvBlastAsset.h"

#include "BlastBaseTest.h"

#include "NvBlastTkFramework.h"

#include <algorithm>


#if defined(_MSC_VER) && _MSC_VER < 1900 || defined(_XBOX_ONE) || defined(PS4) || PX_LINUX
#define ENABLE_SERIALIZATION_TESTS 0
#else
#define ENABLE_SERIALIZATION_TESTS 1
#endif

#pragma warning( push )
#pragma warning( disable : 4267 )
// NOTE: Instead of excluding serialization and the tests when on VC12, should break the tests out into a separate C++ file.

#if ENABLE_SERIALIZATION_TESTS
#include "NvBlastExtSerializationInterface.h"

#include "generated/NvBlastExtSerialization.capn.h"
#endif

#pragma warning( pop )

#include <fstream>
#include <iosfwd>

#ifdef WIN32
#include <windows.h>
#endif

template<int FailLevel, int Verbosity>
class AssetTest : public BlastBaseTest<FailLevel, Verbosity>
{
public:

	AssetTest()
	{
		Nv::Blast::TkFrameworkDesc desc;
		desc.allocatorCallback = this;
		desc.errorCallback = this;
		NvBlastTkFrameworkCreate(desc);
	}

	~AssetTest()
	{
		NvBlastTkFrameworkGet()->release();
	}

	static void messageLog(int type, const char* msg, const char* file, int line)
	{
		BlastBaseTest<FailLevel, Verbosity>::messageLog(type, msg, file, line);
	}

	static void* alloc(size_t size)
	{
		return BlastBaseTest<FailLevel, Verbosity>::alloc(size);
	}

	static void free(void* mem)
	{
		BlastBaseTest<FailLevel, Verbosity>::free(mem);
	}

	void testSubtreeLeafChunkCounts(const Nv::Blast::Asset& a)
	{
		const NvBlastChunk* chunks = a.getChunks();
		const uint32_t* subtreeLeafChunkCounts = a.getSubtreeLeafChunkCounts();
		uint32_t totalLeafChunkCount = 0;
		for (uint32_t chunkIndex = 0; chunkIndex < a.m_chunkCount; ++chunkIndex)
		{
			const NvBlastChunk& chunk = chunks[chunkIndex];
			if (Nv::Blast::isInvalidIndex(chunk.parentChunkIndex))
			{
				totalLeafChunkCount += subtreeLeafChunkCounts[chunkIndex];
			}
			const bool isLeafChunk = chunk.firstChildIndex >= chunk.childIndexStop;
			uint32_t subtreeLeafChunkCount = isLeafChunk ? 1 : 0;
			for (uint32_t childIndex = chunk.firstChildIndex; childIndex < chunk.childIndexStop; ++childIndex)
			{
				subtreeLeafChunkCount += subtreeLeafChunkCounts[childIndex];
			}
			EXPECT_EQ(subtreeLeafChunkCount, subtreeLeafChunkCounts[chunkIndex]);
		}
		EXPECT_EQ(totalLeafChunkCount, a.m_leafChunkCount);
	}

	void testChunkToNodeMap(const Nv::Blast::Asset& a)
	{
		for (uint32_t chunkIndex = 0; chunkIndex < a.m_chunkCount; ++chunkIndex)
		{
			const uint32_t nodeIndex = a.getChunkToGraphNodeMap()[chunkIndex];
			if (!Nv::Blast::isInvalidIndex(nodeIndex))
			{
				EXPECT_LT(nodeIndex, a.m_graph.m_nodeCount);
				EXPECT_EQ(chunkIndex, a.m_graph.getChunkIndices()[nodeIndex]);
			}
			else
			{
				const uint32_t* chunkIndexStop = a.m_graph.getChunkIndices() + a.m_graph.m_nodeCount;
				const uint32_t* it = std::find<const uint32_t*, uint32_t>(a.m_graph.getChunkIndices(), chunkIndexStop, chunkIndex);
				EXPECT_EQ(chunkIndexStop, it);
			}
		}
	}

	NvBlastAsset* buildAsset(const ExpectedAssetValues& expected, const NvBlastAssetDesc* desc)
	{
		std::vector<char> scratch;
		scratch.resize((size_t)NvBlastGetRequiredScratchForCreateAsset(desc, messageLog));
		void* mem = alloc(NvBlastGetAssetMemorySize(desc, messageLog));
		NvBlastAsset* asset = NvBlastCreateAsset(mem, desc, &scratch[0], messageLog);
		EXPECT_TRUE(asset != nullptr);
		if (asset == nullptr)
		{
			free(mem);
			return nullptr;
		}
		Nv::Blast::Asset& a = *(Nv::Blast::Asset*)asset;
		EXPECT_EQ(expected.totalChunkCount, a.m_chunkCount);
		EXPECT_EQ(expected.graphNodeCount, a.m_graph.m_nodeCount);
		EXPECT_EQ(expected.bondCount, a.m_graph.getAdjacencyPartition()[a.m_graph.m_nodeCount] / 2);
		EXPECT_EQ(expected.leafChunkCount, a.m_leafChunkCount);
		EXPECT_EQ(expected.subsupportChunkCount, a.m_chunkCount - a.m_firstSubsupportChunkIndex);
		testSubtreeLeafChunkCounts(a);
		testChunkToNodeMap(a);
		return asset;
	}

	void checkAssetsExpected(Nv::Blast::Asset& asset, const ExpectedAssetValues& expected)
	{
		EXPECT_EQ(expected.totalChunkCount, asset.m_chunkCount);
		EXPECT_EQ(expected.graphNodeCount, asset.m_graph.m_nodeCount);
		EXPECT_EQ(expected.bondCount, asset.m_graph.getAdjacencyPartition()[asset.m_graph.m_nodeCount] / 2);
		EXPECT_EQ(expected.leafChunkCount, asset.m_leafChunkCount);
		EXPECT_EQ(expected.subsupportChunkCount, asset.m_chunkCount - asset.m_firstSubsupportChunkIndex);
		testSubtreeLeafChunkCounts(asset);
		testChunkToNodeMap(asset);
	}

	void buildAssetShufflingDescriptors(const NvBlastAssetDesc* desc, const ExpectedAssetValues& expected, uint32_t shuffleCount, bool useTk)
	{
		NvBlastAssetDesc shuffledDesc = *desc;
		std::vector<NvBlastChunkDesc> chunkDescs(desc->chunkDescs, desc->chunkDescs + desc->chunkCount);
		shuffledDesc.chunkDescs = &chunkDescs[0];
		std::vector<NvBlastBondDesc> bondDescs(desc->bondDescs, desc->bondDescs + desc->bondCount);
		shuffledDesc.bondDescs = &bondDescs[0];
		if (!useTk)
		{
			std::vector<char> scratch(desc->chunkCount);
			NvBlastEnsureAssetExactSupportCoverage(chunkDescs.data(), desc->chunkCount, scratch.data(), messageLog);
		}
		else
		{
			NvBlastTkFrameworkGet()->ensureAssetExactSupportCoverage(chunkDescs.data(), desc->chunkCount);
		}
		for (uint32_t i = 0; i < shuffleCount; ++i)
		{
			shuffleAndFixChunkDescs(&chunkDescs[0], desc->chunkCount, &bondDescs[0], desc->bondCount, useTk);
			NvBlastAsset* asset = buildAsset(expected, &shuffledDesc);
			EXPECT_TRUE(asset != nullptr);
			if (asset)
			{
				free(asset);
			}
		}
	}

	void shuffleAndFixChunkDescs(NvBlastChunkDesc* chunkDescs, uint32_t chunkDescCount, NvBlastBondDesc* bondDescs, uint32_t bondDescCount, bool useTk)
	{
		// Create reorder array and fill with identity map
		std::vector<uint32_t> shuffledOrder(chunkDescCount);
		for (uint32_t i = 0; i < chunkDescCount; ++i)
		{
			shuffledOrder[i] = i;
		}

		// An array into which to copy the reordered descs
		std::vector<NvBlastChunkDesc> shuffledChunkDescs(chunkDescCount);

		std::vector<char> scratch;
		const uint32_t trials = 30;
		uint32_t attempt = 0;
		while(1)
		{
			// Shuffle the reorder array
			std::random_shuffle(shuffledOrder.begin(), shuffledOrder.end());

			// Save initial bonds
			std::vector<NvBlastBondDesc> savedBondDescs(bondDescs, bondDescs + bondDescCount);

			// Shuffle chunks and bonds
			NvBlastApplyAssetDescChunkReorderMap(shuffledChunkDescs.data(), chunkDescs, chunkDescCount, bondDescs, bondDescCount, shuffledOrder.data(), nullptr);

			// Check the results
			for (uint32_t i = 0; i < chunkDescCount; ++i)
			{
				EXPECT_EQ(chunkDescs[i].userData, shuffledChunkDescs[shuffledOrder[i]].userData);
				EXPECT_TRUE(chunkDescs[i].parentChunkIndex > chunkDescCount || shuffledChunkDescs[shuffledOrder[i]].parentChunkIndex == shuffledOrder[chunkDescs[i].parentChunkIndex]);
			}
			for (uint32_t i = 0; i < bondDescCount; ++i)
			{
				for (uint32_t k = 0; k < 2; ++k)
				{
					EXPECT_EQ(shuffledOrder[savedBondDescs[i].chunkIndices[k]], bondDescs[i].chunkIndices[k]);
				}
			}

			// Try creating asset, usually it should fail (otherwise make another attempt)
			NvBlastAssetDesc desc = { chunkDescCount, shuffledChunkDescs.data(), bondDescCount, bondDescs };
			scratch.resize((size_t)NvBlastGetRequiredScratchForCreateAsset(&desc, nullptr));
			void* mem = alloc(NvBlastGetAssetMemorySize(&desc, nullptr));
			NvBlastAsset* asset = NvBlastCreateAsset(mem, &desc, scratch.data(), nullptr);
			if (asset == nullptr)
			{
				free(mem);
				break;
			}
			else
			{
				free(asset);
				memcpy(bondDescs, savedBondDescs.data(), sizeof(NvBlastBondDesc) * bondDescCount);
				attempt++;
				if (attempt >= trials)
				{
					GTEST_NONFATAL_FAILURE_("Shuffled chunk descs should fail asset creation (most of the time).");
					break;
				}
			}
		}

		// Now we want to fix that order
		if (!useTk)
		{
			std::vector<uint32_t> chunkReorderMap(chunkDescCount);
			std::vector<char> scratch2(2 * chunkDescCount * sizeof(uint32_t));
			const bool isIdentity = NvBlastBuildAssetDescChunkReorderMap(chunkReorderMap.data(), shuffledChunkDescs.data(), chunkDescCount, scratch2.data(), messageLog);
			EXPECT_FALSE(isIdentity);
			NvBlastApplyAssetDescChunkReorderMap(chunkDescs, shuffledChunkDescs.data(), chunkDescCount, bondDescs, bondDescCount, chunkReorderMap.data(), messageLog);
		}
		else
		{
			memcpy(chunkDescs, shuffledChunkDescs.data(), chunkDescCount * sizeof(NvBlastChunkDesc));
			const bool isIdentity = NvBlastTkFrameworkGet()->reorderAssetDescChunks(chunkDescs, chunkDescCount, bondDescs, bondDescCount);
			EXPECT_FALSE(isIdentity);
		}
	}
};

typedef AssetTest<NvBlastMessage::Error, 0> AssetTestAllowWarningsSilently;
typedef AssetTest<NvBlastMessage::Error, 1> AssetTestAllowWarnings;
typedef AssetTest<NvBlastMessage::Warning, 1> AssetTestStrict;


TEST_F(AssetTestStrict, BuildAssets)
{
	const uint32_t assetDescCount = sizeof(g_assetDescs) / sizeof(g_assetDescs[0]);

	std::vector<NvBlastAsset*> assets(assetDescCount);

	// Build
	for (uint32_t i = 0; i < assetDescCount; ++i)
	{
		assets[i] = buildAsset(g_assetExpectedValues[i], &g_assetDescs[i]);
	}

	// Destroy
	for (uint32_t i = 0; i < assetDescCount; ++i)
	{
		if (assets[i])
		{
			free(assets[i]);
		}
	}
}

#if ENABLE_SERIALIZATION_TESTS
// Restricting this test to windows since we don't have a handy cross platform temp file.
#if defined(WIN32) || defined(WIN64)
TEST_F(AssetTestStrict, SerializeAssetIntoFile)
{
	const uint32_t assetDescCount = sizeof(g_assetDescs) / sizeof(g_assetDescs[0]);

	std::vector<Nv::Blast::Asset *> assets(assetDescCount);

	// Build
	for (uint32_t i = 0; i < assetDescCount; ++i)
	{
		assets[i] = reinterpret_cast<Nv::Blast::Asset*>(buildAsset(g_assetExpectedValues[i], &g_assetDescs[i]));
	}

	char tempPath[1024];
	GetTempPathA(1024, tempPath);

	char tempFilename[1024];

	GetTempFileNameA(tempPath, nullptr, 0, tempFilename);

	std::ofstream myFile(tempFilename, std::ios::out | std::ios::binary);

	EXPECT_TRUE(serializeAssetIntoStream(assets[0], myFile));

	myFile.flush();

	// Load it back

	std::ifstream myFileReader(tempFilename, std::ios::binary);

	Nv::Blast::Asset* rtAsset = reinterpret_cast<Nv::Blast::Asset *>(deserializeAssetFromStream(myFileReader));
	EXPECT_TRUE(rtAsset != nullptr);

	checkAssetsExpected(*rtAsset, g_assetExpectedValues[0]);

	for (uint32_t i = 0; i < assetDescCount; ++i)
	{
		free(assets[i]);
	}
	free(rtAsset);
}
#endif

TEST_F(AssetTestStrict, SerializeAssetsNewBuffer)
{
	const uint32_t assetDescCount = sizeof(g_assetDescs) / sizeof(g_assetDescs[0]);

	std::vector<Nv::Blast::Asset *> assets(assetDescCount);

	// Build
	for (uint32_t i = 0; i < assetDescCount; ++i)
	{
		assets[i] = reinterpret_cast<Nv::Blast::Asset*>(buildAsset(g_assetExpectedValues[i], &g_assetDescs[i]));
	}

	// Serialize them
	for (Nv::Blast::Asset* asset : assets)
	{
		uint32_t size = 0;
		unsigned char* buffer = nullptr;


//		auto result = Nv::Blast::BlastSerialization<Nv::Blast::Asset, Nv::Blast::Serialization::Asset::Reader, Nv::Blast::Serialization::Asset::Builder>::serializeIntoNewBuffer(asset, &buffer, size);
		EXPECT_TRUE(serializeAssetIntoNewBuffer(asset, &buffer, size));

		free(static_cast<void*>(buffer));
	}

	// Destroy
	for (uint32_t i = 0; i < assetDescCount; ++i)
	{
		if (assets[i])
		{
			free(assets[i]);
		}
	}

}

TEST_F(AssetTestStrict, SerializeAssetsExistingBuffer)
{
	const uint32_t assetDescCount = sizeof(g_assetDescs) / sizeof(g_assetDescs[0]);

	std::vector<Nv::Blast::Asset *> assets(assetDescCount);

	// Build
	for (uint32_t i = 0; i < assetDescCount; ++i)
	{
		assets[i] = reinterpret_cast<Nv::Blast::Asset*>(buildAsset(g_assetExpectedValues[i], &g_assetDescs[i]));
	}

	// How big does our buffer need to be? Guess.

	uint32_t maxSize = 1024 * 1024;
	void* buffer = alloc(maxSize);

	// Serialize them
	for (Nv::Blast::Asset* asset : assets)
	{
		uint32_t usedSize = 0;

		EXPECT_TRUE(serializeAssetIntoExistingBuffer(asset, (unsigned char *)buffer, maxSize, usedSize));
	}

	free(static_cast<void*>(buffer));

	// Destroy
	for (uint32_t i = 0; i < assetDescCount; ++i)
	{
		if (assets[i])
		{
			free(assets[i]);
		}
	}

}

TEST_F(AssetTestStrict, SerializeAssetsRoundTrip)
{
	const uint32_t assetDescCount = sizeof(g_assetDescs) / sizeof(g_assetDescs[0]);

	std::vector<Nv::Blast::Asset *> assets(assetDescCount);

	// Build
	for (uint32_t i = 0; i < assetDescCount; ++i)
	{
		assets[i] = reinterpret_cast<Nv::Blast::Asset*>(buildAsset(g_assetExpectedValues[i], &g_assetDescs[i]));
	}

	// Serialize them
	for (uint32_t i = 0; i < assetDescCount; ++i)
	{
		Nv::Blast::Asset* asset = assets[i];
		uint32_t size = 0;
		unsigned char* buffer = nullptr;

		EXPECT_TRUE(serializeAssetIntoNewBuffer(asset, &buffer, size));

		// No release needed for this asset since it's never put into that system
		Nv::Blast::Asset* rtAsset = reinterpret_cast<Nv::Blast::Asset*>(deserializeAsset(buffer, size));

		//TODO: Compare assets
		checkAssetsExpected(*rtAsset, g_assetExpectedValues[i]);

		free(static_cast<void*>(buffer));
		free(static_cast<void*>(rtAsset));
	}

	// Destroy
	for (uint32_t i = 0; i < assetDescCount; ++i)
	{
		if (assets[i])
		{
			free(assets[i]);
		}
	}
}
#endif


#if 0
TEST_F(AssetTestStrict, AssociateAsset)
{
	const uint32_t assetDescCount = sizeof(g_assetDescs) / sizeof(g_assetDescs[0]);

	for (uint32_t i = 0; i < assetDescCount; ++i)
	{
		// Build
		NvBlastAsset asset;
		if (!buildAsset(&asset, g_assetExpectedValues[i], &g_assetDescs[i]))
		{
			continue;
		}

		// Copy
		const char* data = (const char*)NvBlastAssetGetData(&asset, messageLog);
		const size_t dataSize = NvBlastAssetDataGetSize(data, messageLog);
		NvBlastAsset duplicate;
		char* duplicateData = (char*)alloc(dataSize);
		memcpy(duplicateData, data, dataSize);
		const bool assetAssociateResult = NvBlastAssetAssociateData(&duplicate, duplicateData, messageLog);
		EXPECT_TRUE(assetAssociateResult);

		// Destroy
		NvBlastAssetFreeData(&asset, free, messageLog);
		NvBlastAssetFreeData(&duplicate, free, messageLog);
	}
}
#endif

TEST_F(AssetTestAllowWarnings, BuildAssetsMissingCoverage)
{
	const uint32_t assetDescCount = sizeof(g_assetDescsMissingCoverage) / sizeof(g_assetDescsMissingCoverage[0]);

	std::vector<NvBlastAsset*> assets(assetDescCount);

	// Build
	for (uint32_t i = 0; i < assetDescCount; ++i)
	{
		const NvBlastAssetDesc* desc = &g_assetDescsMissingCoverage[i];
		NvBlastAssetDesc fixedDesc = *desc;
		std::vector<NvBlastChunkDesc> chunkDescs(desc->chunkDescs, desc->chunkDescs + desc->chunkCount);
		std::vector<NvBlastBondDesc> bondDescs(desc->bondDescs, desc->bondDescs + desc->bondCount);
		std::vector<uint32_t> chunkReorderMap(desc->chunkCount);
		std::vector<char> scratch(desc->chunkCount * sizeof(NvBlastChunkDesc));
		const bool changedCoverage = !NvBlastEnsureAssetExactSupportCoverage(chunkDescs.data(), fixedDesc.chunkCount, scratch.data(), messageLog);
		EXPECT_TRUE(changedCoverage);
		NvBlastReorderAssetDescChunks(chunkDescs.data(), fixedDesc.chunkCount, bondDescs.data(), fixedDesc.bondCount, chunkReorderMap.data(), scratch.data(), messageLog);
		fixedDesc.chunkDescs = chunkDescs.data();
		fixedDesc.bondDescs = bondDescs.data();
		assets[i] = buildAsset(g_assetsFromMissingCoverageExpectedValues[i], &fixedDesc);
	}

	// Destroy
	for (uint32_t i = 0; i < assetDescCount; ++i)
	{
		if (assets[i])
		{
			free(assets[i]);
		}
	}
}

TEST_F(AssetTestAllowWarningsSilently, BuildAssetsShufflingChunkDescriptors)
{
	for (uint32_t i = 0; i < sizeof(g_assetDescs) / sizeof(g_assetDescs[0]); ++i)
	{
		buildAssetShufflingDescriptors(&g_assetDescs[i], g_assetExpectedValues[i], 10, false);
	}

	for (uint32_t i = 0; i < sizeof(g_assetDescsMissingCoverage) / sizeof(g_assetDescsMissingCoverage[0]); ++i)
	{
		buildAssetShufflingDescriptors(&g_assetDescsMissingCoverage[i], g_assetsFromMissingCoverageExpectedValues[i], 10, false);
	}
}

TEST_F(AssetTestAllowWarningsSilently, BuildAssetsShufflingChunkDescriptorsUsingTk)
{
	for (uint32_t i = 0; i < sizeof(g_assetDescs) / sizeof(g_assetDescs[0]); ++i)
	{
		buildAssetShufflingDescriptors(&g_assetDescs[i], g_assetExpectedValues[i], 10, true);
	}

	for (uint32_t i = 0; i < sizeof(g_assetDescsMissingCoverage) / sizeof(g_assetDescsMissingCoverage[0]); ++i)
	{
		buildAssetShufflingDescriptors(&g_assetDescsMissingCoverage[i], g_assetsFromMissingCoverageExpectedValues[i], 10, true);
	}
}
