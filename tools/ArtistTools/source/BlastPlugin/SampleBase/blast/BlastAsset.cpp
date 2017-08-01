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
// Copyright (c) 2008-2017 NVIDIA Corporation. All rights reserved.


#include "BlastAsset.h"
#include "NvBlastExtPxAsset.h"
#include "NvBlastTkAsset.h"
#include <map>
#include <algorithm>
// Add By Lixu Begin
#include "BlastSceneTree.h"
#include "SampleManager.h"
// Add By Lixu End


BlastAsset::BlastAsset(Renderer& renderer)
	: m_renderer(renderer), m_bondHealthMax(1.0f), m_supportChunkHealthMax(1.0f)
{
}

void BlastAsset::initialize()
{
// Add By Lixu Begin
	BPPBlast& blast = BlastProject::ins().getParams().blast;
	BPPChunkArray& chunks = blast.chunks;
	BPPBondArray& bonds = blast.bonds;

	std::vector<float> BondHealths;
	SampleManager* pSampleManager = SampleManager::ins();
	std::map<BlastAsset*, AssetList::ModelAsset>& AssetDescMap = pSampleManager->getAssetDescMap();
	std::map<BlastAsset*, AssetList::ModelAsset>::iterator itASM = AssetDescMap.find(this);
	AssetList::ModelAsset m;
	if (itASM != AssetDescMap.end())
	{
		m = itASM->second;
	}

	int assetID = BlastProject::ins().getAssetIDByName(m.name.c_str());
	for (int bc = 0; bc < bonds.arraySizes[0]; bc++)
	{
		BPPBond& bond = bonds.buf[bc];
		if (bond.asset == assetID)
		{
			BondHealths.push_back(bond.support.bondStrength);
		}
	}

	const TkAsset& tkAsset = m_pxAsset->getTkAsset();
	uint32_t bondCount = tkAsset.getBondCount();

	const float* pBondHealths = nullptr;
	if (bondCount == BondHealths.size())
	{
		pBondHealths = BondHealths.data();
	}

	const NvBlastActorDesc& defaultActorDesc = m_pxAsset->getDefaultActorDesc();
	NvBlastActorDesc newActorDesc = defaultActorDesc;
	newActorDesc.initialBondHealths = pBondHealths;
// Add By Lixu End

	// calc max healths
	const NvBlastActorDesc& actorDesc = newActorDesc;
	if (actorDesc.initialBondHealths)
	{
		m_bondHealthMax = FLT_MIN;
		const uint32_t bondCount = m_pxAsset->getTkAsset().getBondCount();
		for (uint32_t i = 0; i < bondCount; ++i)
		{
			m_bondHealthMax = std::max<float>(m_bondHealthMax, actorDesc.initialBondHealths[i]);
		}
	}
	else
	{
		m_bondHealthMax = actorDesc.uniformInitialBondHealth;
	}

	if(actorDesc.initialSupportChunkHealths)
	{
		m_supportChunkHealthMax = FLT_MIN;
		const uint32_t nodeCount = m_pxAsset->getTkAsset().getGraph().nodeCount;
		for (uint32_t i = 0; i < nodeCount; ++i)
		{
			m_supportChunkHealthMax = std::max<float>(m_supportChunkHealthMax, actorDesc.initialSupportChunkHealths[i]);
		}
	}
	else
	{
		m_supportChunkHealthMax = actorDesc.uniformInitialLowerSupportChunkHealth;
	}
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
