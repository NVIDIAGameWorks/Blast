/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

#include "NvBlastExtSerialization.h"
#include "NvBlastAsset.h"
#include "AssetDTO.h"

namespace Nv
{
	namespace Blast
	{
		/*
		Specializations here - LL asset only
		*/
		
		// Asset
		template<>
		NV_INLINE bool ExtSerialization<Nv::Blast::Asset, Nv::Blast::Serialization::Asset::Reader, Nv::Blast::Serialization::Asset::Builder>::serializeIntoBuilder(Nv::Blast::Serialization::Asset::Builder& assetBuilder, const Nv::Blast::Asset* asset)
		{
			return AssetDTO::serialize(assetBuilder, asset);
		}
		
		template<>
		NV_INLINE Nv::Blast::Asset* ExtSerialization<Nv::Blast::Asset, Nv::Blast::Serialization::Asset::Reader, Nv::Blast::Serialization::Asset::Builder>::deserializeFromStreamReader(capnp::InputStreamMessageReader &message)
		{
			Nv::Blast::Serialization::Asset::Reader reader = message.getRoot<Nv::Blast::Serialization::Asset>();
		
			return AssetDTO::deserialize(reader);
		}
		
		template<>
		NV_INLINE bool ExtSerialization<Nv::Blast::Asset, Nv::Blast::Serialization::Asset::Reader, Nv::Blast::Serialization::Asset::Builder>::serializeIntoMessage(capnp::MallocMessageBuilder& message, const Nv::Blast::Asset* asset)
		{
			Nv::Blast::Serialization::Asset::Builder assetBuilder = message.initRoot<Nv::Blast::Serialization::Asset>();
		
			return serializeIntoBuilder(assetBuilder, asset);
		}
	}
}
