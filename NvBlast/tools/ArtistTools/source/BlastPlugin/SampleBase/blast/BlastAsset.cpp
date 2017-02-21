/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "BlastAsset.h"
#include "NvBlastExtPxAsset.h"
#include "NvBlastTkAsset.h"
#include <map>

BlastAsset::BlastAsset(Renderer& renderer)
	: m_renderer(renderer)
{
}

void BlastAsset::validate()
{
}

size_t BlastAsset::getBlastAssetSize() const
{
	return m_pxAsset->getTkAsset().getDataSize();
}

std::vector<uint32_t> BlastAsset::getChunkIndexesByDepth(uint32_t depth) const
{
	const TkAsset& tkAsset = m_pxAsset->getTkAsset();
	const NvBlastChunk* pNvBlastChunk = tkAsset.getChunks();
	uint32_t chunkCount = tkAsset.getChunkCount();

	std::map<uint32_t, uint32_t> indexDepthMap;
	indexDepthMap.insert(std::make_pair(0, 0));
	for (size_t i = 1; i < chunkCount; ++i)
	{
		int depth = 0;
		const NvBlastChunk* curChunk = pNvBlastChunk + i;
		while (-1 != curChunk->parentChunkIndex)
		{
			++depth;
			curChunk = pNvBlastChunk + curChunk->parentChunkIndex;
		}
		indexDepthMap.insert(std::make_pair(i, depth));
	}

	std::vector<uint32_t> indexes;
	for (std::map<uint32_t, uint32_t>::iterator itr = indexDepthMap.begin(); itr != indexDepthMap.end(); ++itr)
	{
		if (itr->second == depth)
		{
			indexes.push_back(itr->first);
		}
	}

	return indexes;
}
