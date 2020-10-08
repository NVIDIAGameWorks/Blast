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
// Copyright (c) 2020 NVIDIA Corporation. All rights reserved.


#include "ExtPxAssetDTO.h"
#include "TkAssetDTO.h"
#include "ExtPxChunkDTO.h"
#include "ExtPxSubchunkDTO.h"
#include "physics/NvBlastExtPxAssetImpl.h"
#include "NvBlastAssert.h"
#include "NvBlast.h"


namespace Nv
{
namespace Blast
{

bool ExtPxAssetDTO::serialize(Nv::Blast::Serialization::ExtPxAsset::Builder builder, const Nv::Blast::ExtPxAsset * poco)
{
	TkAssetDTO::serialize(builder.getAsset(), &poco->getTkAsset());

	auto chunks = builder.initChunks(poco->getChunkCount());

	for (uint32_t i = 0; i <poco->getChunkCount(); i++)
	{
		ExtPxChunkDTO::serialize(chunks[i], &poco->getChunks()[i]);
	}

	auto subchunks = builder.initSubchunks(poco->getSubchunkCount());

	for (uint32_t i = 0; i < poco->getSubchunkCount(); i++)
	{
		ExtPxSubchunkDTO::serialize(subchunks[i], &poco->getSubchunks()[i]);
	}

	const NvBlastActorDesc& actorDesc = poco->getDefaultActorDesc();

	builder.setUniformInitialBondHealth(actorDesc.uniformInitialBondHealth);

	if (actorDesc.initialBondHealths != nullptr)
	{
		const uint32_t bondCount = poco->getTkAsset().getBondCount();
		kj::ArrayPtr<const float> bondHealthArray(actorDesc.initialBondHealths, bondCount);
		builder.initBondHealths(bondCount);
		builder.setBondHealths(bondHealthArray);
	}

	builder.setUniformInitialLowerSupportChunkHealth(actorDesc.uniformInitialLowerSupportChunkHealth);

	if (actorDesc.initialSupportChunkHealths != nullptr)
	{
		const uint32_t supportChunkCount = NvBlastAssetGetSupportChunkCount(poco->getTkAsset().getAssetLL(), logLL);
		kj::ArrayPtr<const float> supportChunkHealthArray(actorDesc.initialSupportChunkHealths, supportChunkCount);
		builder.initSupportChunkHealths(supportChunkCount);
		builder.setSupportChunkHealths(supportChunkHealthArray);
	}

	return true;
}


Nv::Blast::ExtPxAsset* ExtPxAssetDTO::deserialize(Nv::Blast::Serialization::ExtPxAsset::Reader reader)
{
	auto tkAsset = TkAssetDTO::deserialize(reader.getAsset());

	Nv::Blast::ExtPxAssetImpl* asset = reinterpret_cast<Nv::Blast::ExtPxAssetImpl*>(Nv::Blast::ExtPxAsset::create(tkAsset));

	NVBLAST_ASSERT(asset != nullptr);

	auto& chunks = asset->getChunksArray();
	const uint32_t chunkCount = reader.getChunks().size();
	chunks.resize(chunkCount);
	auto readerChunks = reader.getChunks();
	for (uint32_t i = 0; i < chunkCount; i++)
	{
		ExtPxChunkDTO::deserializeInto(readerChunks[i], &chunks[i]);
	}

	auto& subchunks = asset->getSubchunksArray();
	const uint32_t subChunkCount = reader.getSubchunks().size();
	subchunks.resize(subChunkCount);
	auto readerSubchunks = reader.getSubchunks();
	for (uint32_t i = 0; i < subChunkCount; i++)
	{
		ExtPxSubchunkDTO::deserializeInto(readerSubchunks[i], &subchunks[i]);
	}

	NvBlastActorDesc& actorDesc = asset->getDefaultActorDesc();

	actorDesc.uniformInitialBondHealth = reader.getUniformInitialBondHealth();

	actorDesc.initialBondHealths = nullptr;
	if (reader.hasBondHealths())
	{
		const uint32_t bondCount = asset->getTkAsset().getBondCount();
		Nv::Blast::Array<float>::type& bondHealths = asset->getBondHealthsArray();
		bondHealths.resize(bondCount);
		auto readerBondHealths = reader.getBondHealths();
		for (uint32_t i = 0; i < bondCount; ++i)
		{
			bondHealths[i] = readerBondHealths[i];
		}
	}

	actorDesc.uniformInitialLowerSupportChunkHealth = reader.getUniformInitialLowerSupportChunkHealth();

	actorDesc.initialSupportChunkHealths = nullptr;
	if (reader.hasSupportChunkHealths())
	{
		const uint32_t supportChunkCount = NvBlastAssetGetSupportChunkCount(asset->getTkAsset().getAssetLL(), logLL);
		Nv::Blast::Array<float>::type& supportChunkHealths = asset->getSupportChunkHealthsArray();
		supportChunkHealths.resize(supportChunkCount);
		auto readerSupportChunkHealths = reader.getSupportChunkHealths();
		for (uint32_t i = 0; i < supportChunkCount; ++i)
		{
			supportChunkHealths[i] = readerSupportChunkHealths[i];
		}
	}

	return asset;
}


bool ExtPxAssetDTO::deserializeInto(Nv::Blast::Serialization::ExtPxAsset::Reader reader, Nv::Blast::ExtPxAsset * poco)
{
	NV_UNUSED(reader);
	poco = nullptr;
	//NOTE: Because of the way this is structured, can't do this.
	return false;
}

}	// namespace Blast
}	// namespace Nv
