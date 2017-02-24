/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "ExtPxAssetDTO.h"
#include "TkAssetDTO.h"
#include "ExtPxChunkDTO.h"
#include "ExtPxSubchunkDTO.h"
#include "physics/NvBlastExtPxAssetImpl.h"
#include "NvBlastAssert.h"

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
			
			return true;
		}
		
		Nv::Blast::ExtPxAsset* ExtPxAssetDTO::deserialize(Nv::Blast::Serialization::ExtPxAsset::Reader reader)
		{
			auto tkAsset = TkAssetDTO::deserialize(reader.getAsset());
		
			Nv::Blast::ExtPxAssetImpl* asset = reinterpret_cast<Nv::Blast::ExtPxAssetImpl*>(Nv::Blast::ExtPxAsset::create(tkAsset));
		
			NVBLAST_ASSERT(asset != nullptr);
		
			auto chunks = asset->getChunksArray();
		
			chunks.resize(reader.getChunks().size());
			for (uint32_t i = 0; i < reader.getChunks().size(); i++)
			{
				ExtPxChunkDTO::deserializeInto(reader.getChunks()[i], &chunks[i]);
			}
		
			auto subchunks = asset->getSubchunksArray();
		
			subchunks.resize(reader.getSubchunks().size());
			for (uint32_t i = 0; i < reader.getSubchunks().size(); i++)
			{
				ExtPxSubchunkDTO::deserializeInto(reader.getSubchunks()[i], &subchunks[i]);
			}
		
			return asset;
		}
		
		bool ExtPxAssetDTO::deserializeInto(Nv::Blast::Serialization::ExtPxAsset::Reader reader, Nv::Blast::ExtPxAsset * poco)
		{
			reader = reader;
			poco = nullptr;
			//NOTE: Because of the way this is structured, can't do this.
			return false;
		}
	}
}
