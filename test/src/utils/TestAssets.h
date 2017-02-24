#ifndef TESTASSETS_H
#define TESTASSETS_H

#include "NvBlast.h"
#include "AssetGenerator.h"

struct ExpectedAssetValues
{
	uint32_t	totalChunkCount;
	uint32_t	graphNodeCount;
	uint32_t	leafChunkCount;
	uint32_t	bondCount;
	uint32_t	subsupportChunkCount;
};


// Indexable asset descriptors and expected values
extern const NvBlastAssetDesc g_assetDescs[3];
extern const ExpectedAssetValues g_assetExpectedValues[3];

// Indexable asset descriptors for assets missing coverage and expected values
extern const NvBlastAssetDesc g_assetDescsMissingCoverage[3];
extern const ExpectedAssetValues g_assetsFromMissingCoverageExpectedValues[3];


inline uint32_t getAssetDescCount()
{
	return sizeof(g_assetDescs) / sizeof(g_assetDescs[0]);
}

inline uint32_t getAssetDescMissingCoverageCount()
{
	return sizeof(g_assetDescsMissingCoverage) / sizeof(g_assetDescsMissingCoverage[0]);
}


void generateCube(GeneratorAsset& cubeAsset, size_t maxDepth, size_t width, int32_t supportDepth = -1);
void generateCube(GeneratorAsset& cubeAsset, NvBlastAssetDesc& assetDesc, size_t maxDepth, size_t width, int32_t supportDepth = -1);

#endif // #ifdef TESTASSETS_H
