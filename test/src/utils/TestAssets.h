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
extern const NvBlastAssetDesc g_assetDescs[6];
extern const ExpectedAssetValues g_assetExpectedValues[6];

// Indexable asset descriptors for assets missing coverage and expected values
extern const NvBlastAssetDesc g_assetDescsMissingCoverage[6];
extern const ExpectedAssetValues g_assetsFromMissingCoverageExpectedValues[6];


inline uint32_t getAssetDescCount()
{
	return sizeof(g_assetDescs) / sizeof(g_assetDescs[0]);
}

inline uint32_t getAssetDescMissingCoverageCount()
{
	return sizeof(g_assetDescsMissingCoverage) / sizeof(g_assetDescsMissingCoverage[0]);
}


void generateCube(GeneratorAsset& cubeAsset, NvBlastAssetDesc& assetDesc, size_t maxDepth, size_t width, 
	int32_t supportDepth = -1, CubeAssetGenerator::BondFlags bondFlags = CubeAssetGenerator::ALL_INTERNAL_BONDS);

void generateRandomCube(GeneratorAsset& cubeAsset, NvBlastAssetDesc& assetDesc, uint32_t minChunkCount, uint32_t maxChunkCount);

#endif // #ifdef TESTASSETS_H
