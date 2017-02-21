/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/


#include "NvBlastTkFrameworkImpl.h"
#include "NvBlastTkAssetImpl.h"
#include "NvBlastTkFamilyImpl.h"

#include "NvBlast.h"
#include "NvBlastMemory.h"

#include "Px.h"
#include "PxFileBuf.h"
#include "PxAllocatorCallback.h"


using namespace physx::general_PxIOStream2;


namespace Nv
{
namespace Blast
{

//////// Static data ////////

NVBLASTTK_DEFINE_TYPE_SERIALIZABLE(Asset);


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
		TkFrameworkImpl::get()->free(m_assetLL);
	}
}


const NvBlastAsset* TkAssetImpl::getAssetLL() const
{
	return getAssetLLInternal();
}


uint32_t TkAssetImpl::getChunkCount() const
{
	return NvBlastAssetGetChunkCount(m_assetLL, TkFrameworkImpl::get()->log);
}


uint32_t TkAssetImpl::getLeafChunkCount() const
{
	return NvBlastAssetGetLeafChunkCount(m_assetLL, TkFrameworkImpl::get()->log);
}


uint32_t TkAssetImpl::getBondCount() const
{
	return NvBlastAssetGetBondCount(m_assetLL, TkFrameworkImpl::get()->log);
}


const NvBlastChunk* TkAssetImpl::getChunks() const
{
	return NvBlastAssetGetChunks(m_assetLL, TkFrameworkImpl::get()->log);
}


const NvBlastBond* TkAssetImpl::getBonds() const
{
	return NvBlastAssetGetBonds(m_assetLL, TkFrameworkImpl::get()->log);
}


const NvBlastSupportGraph TkAssetImpl::getGraph() const
{
	return NvBlastAssetGetSupportGraph(m_assetLL, TkFrameworkImpl::get()->log);
}


uint32_t TkAssetImpl::getDataSize() const
{
	return NvBlastAssetGetSize(m_assetLL, TkFrameworkImpl::get()->log);
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
		TkArray<TkIdentifiable*>::type dependents(num);
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

	NVBLASTTK_DELETE(this, TkAssetImpl);
}


bool TkAssetImpl::serialize(PxFileBuf& stream) const
{
	TkFrameworkImpl::get()->serializeHeader(*this, stream);

	// Asset data
	const uint32_t assetSize = NvBlastAssetGetSize(m_assetLL, TkFrameworkImpl::get()->log);
	stream.storeDword(assetSize);
	stream.write(m_assetLL, assetSize);

	// Joint descs
	stream.storeDword((uint32_t)m_jointDescs.size());
	for (uint32_t i = 0; i < m_jointDescs.size(); ++i)
	{
		const TkAssetJointDesc& jointDesc = m_jointDescs[i];
		stream.storeDword(jointDesc.nodeIndices[0]);
		stream.storeDword(jointDesc.nodeIndices[1]);
		stream.storeFloat(jointDesc.attachPositions[0].x);
		stream.storeFloat(jointDesc.attachPositions[0].y);
		stream.storeFloat(jointDesc.attachPositions[0].z);
		stream.storeFloat(jointDesc.attachPositions[1].x);
		stream.storeFloat(jointDesc.attachPositions[1].y);
		stream.storeFloat(jointDesc.attachPositions[1].z);
	}

	return true;
}


//////// Static functions ////////

TkSerializable* TkAssetImpl::deserialize(PxFileBuf& stream, const NvBlastID& id)
{
	// Allocate
	TkAssetImpl* asset = NVBLASTTK_NEW(TkAssetImpl)(id);
	if (asset == nullptr)
	{
		NVBLASTTK_LOG_ERROR("TkAssetImpl::deserialize: asset allocation failed.");
		return nullptr;
	}

	// Asset data
 	const uint32_t assetSize = stream.readDword();
	asset->m_assetLL = static_cast<NvBlastAsset*>(TkFrameworkImpl::get()->alloc(assetSize));
	asset->m_ownsAsset = true;
	stream.read(asset->m_assetLL, assetSize);

	// Joint descs
	const uint32_t jointDescCount = stream.readDword();
	asset->m_jointDescs.resize(jointDescCount);
	for (uint32_t i = 0; i < asset->m_jointDescs.size(); ++i)
	{
		TkAssetJointDesc& jointDesc = asset->m_jointDescs[i];
		jointDesc.nodeIndices[0] = stream.readDword();
		jointDesc.nodeIndices[1] = stream.readDword();
		jointDesc.attachPositions[0].x = stream.readFloat();
		jointDesc.attachPositions[0].y = stream.readFloat();
		jointDesc.attachPositions[0].z = stream.readFloat();
		jointDesc.attachPositions[1].x = stream.readFloat();
		jointDesc.attachPositions[1].y = stream.readFloat();
		jointDesc.attachPositions[1].z = stream.readFloat();
	}


	if (asset->m_assetLL == nullptr)
	{
		asset->release();
		asset = nullptr;
	}

	return asset;
}


TkAssetImpl* TkAssetImpl::create(const TkAssetDesc& desc)
{
	TkAssetImpl* asset = NVBLASTTK_NEW(TkAssetImpl);

	TkArray<char>::type scratch((uint32_t)NvBlastGetRequiredScratchForCreateAsset(&desc, TkFrameworkImpl::get()->log));
	void* mem = TkFrameworkImpl::get()->alloc(NvBlastGetAssetMemorySize(&desc, TkFrameworkImpl::get()->log));
	asset->m_assetLL = NvBlastCreateAsset(mem, &desc, scratch.begin(), TkFrameworkImpl::get()->log);
	if (asset->m_assetLL == nullptr)
	{
		NVBLASTTK_LOG_ERROR("TkAssetImpl::create: low-level asset could not be created.");
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
					NVBLASTTK_LOG_WARNING("TkAssetImpl::create: joint flag set for badly described bond.  No joint descriptor created.");
					continue;
				}

				if (!asset->addJointDesc(c0, c1))
				{
					NVBLASTTK_LOG_WARNING("TkAssetImpl::create: no bond corresponds to the user-described bond indices.  No joint descriptor created.");
				}
			}
		}
	}

	asset->m_ownsAsset = true;
//	asset->setID(NvBlastAssetGetID(asset->m_assetLL, TkFrameworkImpl::get()->log));	// Keeping LL and Tk IDs distinct

	return asset;
}


TkAssetImpl* TkAssetImpl::create(const NvBlastAsset* assetLL, Nv::Blast::TkAssetJointDesc* jointDescs, uint32_t jointDescCount, bool ownsAsset)
{
	TkAssetImpl* asset = NVBLASTTK_NEW(TkAssetImpl);

	//NOTE: Why are we passing in a const NvBlastAsset* and then discarding the const?
	asset->m_assetLL = const_cast<NvBlastAsset*>(assetLL);
	if (asset->m_assetLL == nullptr)
	{
		NVBLASTTK_LOG_ERROR("TkAssetImpl::create: low-level asset could not be created.");
		asset->release();
		return nullptr;
	}

	asset->m_ownsAsset = ownsAsset;
	asset->setID(NvBlastAssetGetID(asset->m_assetLL, TkFrameworkImpl::get()->log));

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

	const uint32_t upperSupportChunkCount = NvBlastAssetGetFirstSubsupportChunkIndex(m_assetLL, TkFrameworkImpl::get()->log);
	if (chunkIndex0 >= upperSupportChunkCount || chunkIndex1 >= upperSupportChunkCount)
	{
		return false;
	}

	const uint32_t* chunkToGraphNodeMap = NvBlastAssetGetChunkToGraphNodeMap(m_assetLL, TkFrameworkImpl::get()->log);
	const uint32_t node0 = chunkToGraphNodeMap[chunkIndex0];
	const uint32_t node1 = chunkToGraphNodeMap[chunkIndex1];
	const NvBlastSupportGraph graph = NvBlastAssetGetSupportGraph(m_assetLL, TkFrameworkImpl::get()->log);
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

	if (bondIndex >= NvBlastAssetGetBondCount(m_assetLL, TkFrameworkImpl::get()->log))
	{
		return false;
	}

	const NvBlastBond& bond = NvBlastAssetGetBonds(m_assetLL, TkFrameworkImpl::get()->log)[bondIndex];

	TkAssetJointDesc jointDesc;
	jointDesc.attachPositions[0] = jointDesc.attachPositions[1] = physx::PxVec3(bond.centroid[0], bond.centroid[1], bond.centroid[2]);
	jointDesc.nodeIndices[0] = node0;
	jointDesc.nodeIndices[1] = node1;
	m_jointDescs.pushBack(jointDesc);

	return true;
}

} // namespace Blast
} // namespace Nv
