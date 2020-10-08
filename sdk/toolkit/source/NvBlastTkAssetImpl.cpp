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
// Copyright (c) 2016-2020 NVIDIA Corporation. All rights reserved.


#include "NvBlastTkFrameworkImpl.h"
#include "NvBlastTkAssetImpl.h"
#include "NvBlastTkFamilyImpl.h"

#include "NvBlast.h"
#include "NvBlastMemory.h"


namespace Nv
{
namespace Blast
{

//////// Static data ////////

NVBLASTTK_DEFINE_TYPE_IDENTIFIABLE(Asset);


//////// Member functions ////////

TkAssetImpl::TkAssetImpl()
	: m_assetLL(nullptr), m_ownsAsset(false)
{
}


TkAssetImpl::TkAssetImpl(const NvBlastID& id)
	: TkAssetType(id), m_assetLL(nullptr), m_ownsAsset(false)
{
}


TkAssetImpl::~TkAssetImpl()
{
	if (m_assetLL != nullptr && m_ownsAsset)
	{
		NVBLAST_FREE(m_assetLL);
	}
}


const NvBlastAsset* TkAssetImpl::getAssetLL() const
{
	return getAssetLLInternal();
}


uint32_t TkAssetImpl::getChunkCount() const
{
	return NvBlastAssetGetChunkCount(m_assetLL, logLL);
}


uint32_t TkAssetImpl::getLeafChunkCount() const
{
	return NvBlastAssetGetLeafChunkCount(m_assetLL, logLL);
}


uint32_t TkAssetImpl::getBondCount() const
{
	return NvBlastAssetGetBondCount(m_assetLL, logLL);
}


const NvBlastChunk* TkAssetImpl::getChunks() const
{
	return NvBlastAssetGetChunks(m_assetLL, logLL);
}


const NvBlastBond* TkAssetImpl::getBonds() const
{
	return NvBlastAssetGetBonds(m_assetLL, logLL);
}


const NvBlastSupportGraph TkAssetImpl::getGraph() const
{
	return NvBlastAssetGetSupportGraph(m_assetLL, logLL);
}


uint32_t TkAssetImpl::getDataSize() const
{
	return NvBlastAssetGetSize(m_assetLL, logLL);
}


uint32_t TkAssetImpl::getJointDescCount() const
{
	return getJointDescCountInternal();
}


const TkAssetJointDesc* TkAssetImpl::getJointDescs() const
{
	return getJointDescsInternal();
}


void TkAssetImpl::release()
{
	const TkType& tkType = TkFamilyImpl::s_type;
	const uint32_t num = TkFrameworkImpl::get()->getObjectCount(tkType);

	if (num)
	{
		Array<TkIdentifiable*>::type dependents(num);
		TkFrameworkImpl::get()->getObjects(dependents.begin(), dependents.size(), tkType);

		for (TkObject* o : dependents)
		{
			TkFamilyImpl* f = static_cast<TkFamilyImpl*>(o);
			if (f->getAssetImpl() == this)
			{
				f->release();
			}
		}
	}

	NVBLAST_DELETE(this, TkAssetImpl);
}


//////// Static functions ////////

TkAssetImpl* TkAssetImpl::create(const TkAssetDesc& desc)
{
	TkAssetImpl* asset = NVBLAST_NEW(TkAssetImpl);

	Array<char>::type scratch((uint32_t)NvBlastGetRequiredScratchForCreateAsset(&desc, logLL));
	void* mem = NVBLAST_ALLOC_NAMED(NvBlastGetAssetMemorySize(&desc, logLL), "TkAssetImpl::create");
	asset->m_assetLL = NvBlastCreateAsset(mem, &desc, scratch.begin(), logLL);
	if (asset->m_assetLL == nullptr)
	{
		NVBLAST_LOG_ERROR("TkAssetImpl::create: low-level asset could not be created.");
		asset->release();
		return nullptr;
	}

	if (desc.bondFlags != nullptr)
	{
		for (uint32_t bondN = 0; bondN < desc.bondCount; ++bondN)
		{
			if (0 != (desc.bondFlags[bondN] & TkAssetDesc::BondJointed))
			{
				const NvBlastBondDesc& bondDesc = desc.bondDescs[bondN];
				const uint32_t c0 = bondDesc.chunkIndices[0];
				const uint32_t c1 = bondDesc.chunkIndices[1];
				if (c0 >= desc.chunkCount || c1 >= desc.chunkCount)
				{
					NVBLAST_LOG_WARNING("TkAssetImpl::create: joint flag set for badly described bond.  No joint descriptor created.");
					continue;
				}

				if (!asset->addJointDesc(c0, c1))
				{
					NVBLAST_LOG_WARNING("TkAssetImpl::create: no bond corresponds to the user-described bond indices.  No joint descriptor created.");
				}
			}
		}
	}

	asset->m_ownsAsset = true;
//	asset->setID(NvBlastAssetGetID(asset->m_assetLL, logLL));	// Keeping LL and Tk IDs distinct

	return asset;
}


TkAssetImpl* TkAssetImpl::create(const NvBlastAsset* assetLL, Nv::Blast::TkAssetJointDesc* jointDescs, uint32_t jointDescCount, bool ownsAsset)
{
	TkAssetImpl* asset = NVBLAST_NEW(TkAssetImpl);

	//NOTE: Why are we passing in a const NvBlastAsset* and then discarding the const?
	asset->m_assetLL = const_cast<NvBlastAsset*>(assetLL);
	if (asset->m_assetLL == nullptr)
	{
		NVBLAST_LOG_ERROR("TkAssetImpl::create: low-level asset could not be created.");
		asset->release();
		return nullptr;
	}

	asset->m_ownsAsset = ownsAsset;
	asset->setID(NvBlastAssetGetID(asset->m_assetLL, logLL));

	asset->m_jointDescs.resize(jointDescCount);
	for (uint32_t i = 0; i < asset->m_jointDescs.size(); ++i)
	{
		asset->m_jointDescs[i] = jointDescs[i];
	}

	return asset;
}

bool TkAssetImpl::addJointDesc(uint32_t chunkIndex0, uint32_t chunkIndex1)
{
	if (m_assetLL == nullptr)
	{
		return false;
	}

	const uint32_t upperSupportChunkCount = NvBlastAssetGetFirstSubsupportChunkIndex(m_assetLL, logLL);
	if (chunkIndex0 >= upperSupportChunkCount || chunkIndex1 >= upperSupportChunkCount)
	{
		return false;
	}

	const uint32_t* chunkToGraphNodeMap = NvBlastAssetGetChunkToGraphNodeMap(m_assetLL, logLL);
	const uint32_t node0 = chunkToGraphNodeMap[chunkIndex0];
	const uint32_t node1 = chunkToGraphNodeMap[chunkIndex1];
	const NvBlastSupportGraph graph = NvBlastAssetGetSupportGraph(m_assetLL, logLL);
	if (node0 >= graph.nodeCount && node1 >= graph.nodeCount)
	{
		return false;
	}

	// Find bond index
	// Iterate through all neighbors of node0 chunk
	uint32_t bondIndex = 0xFFFFFFFF;
	for (uint32_t i = graph.adjacencyPartition[node0]; i < graph.adjacencyPartition[node0 + 1]; i++)
	{
		if (graph.adjacentNodeIndices[i] == node1)
		{
			bondIndex = graph.adjacentBondIndices[i];
			break;
		}
	}

	if (bondIndex >= NvBlastAssetGetBondCount(m_assetLL, logLL))
	{
		return false;
	}

	const NvBlastBond& bond = NvBlastAssetGetBonds(m_assetLL, logLL)[bondIndex];

	TkAssetJointDesc jointDesc;
	jointDesc.attachPositions[0] = jointDesc.attachPositions[1] = physx::PxVec3(bond.centroid[0], bond.centroid[1], bond.centroid[2]);
	jointDesc.nodeIndices[0] = node0;
	jointDesc.nodeIndices[1] = node1;
	m_jointDescs.pushBack(jointDesc);

	return true;
}

} // namespace Blast
} // namespace Nv
