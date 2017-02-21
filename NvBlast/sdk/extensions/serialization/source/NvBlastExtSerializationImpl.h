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
#include "NvBlastTkAsset.h"
#include "NvBlastExtPxAsset.h"
#include "TkAssetDTO.h"
#include "ExtPxAssetDTO.h"

namespace Nv
{
	namespace Blast
	{
		/*
		Specializations here - one set for each top level asset. (TkAsset, ExtPxAsset)
		*/
		
	
		// TkAsset
		template<>
		NV_INLINE bool ExtSerialization<Nv::Blast::TkAsset, Nv::Blast::Serialization::TkAsset::Reader, Nv::Blast::Serialization::TkAsset::Builder>::serializeIntoBuilder(Nv::Blast::Serialization::TkAsset::Builder& assetBuilder, const Nv::Blast::TkAsset* asset)
		{
			return TkAssetDTO::serialize(assetBuilder, asset);
		}
		
		template<>
		NV_INLINE Nv::Blast::TkAsset* ExtSerialization<Nv::Blast::TkAsset, Nv::Blast::Serialization::TkAsset::Reader, Nv::Blast::Serialization::TkAsset::Builder>::deserializeFromStreamReader(capnp::InputStreamMessageReader &message)
		{
			Nv::Blast::Serialization::TkAsset::Reader reader = message.getRoot<Nv::Blast::Serialization::TkAsset>();
		
			return TkAssetDTO::deserialize(reader);
		}
		
		template<>
		NV_INLINE bool ExtSerialization<Nv::Blast::TkAsset, Nv::Blast::Serialization::TkAsset::Reader, Nv::Blast::Serialization::TkAsset::Builder>::serializeIntoMessage(capnp::MallocMessageBuilder& message, const Nv::Blast::TkAsset* asset)
		{
			Nv::Blast::Serialization::TkAsset::Builder assetBuilder = message.initRoot<Nv::Blast::Serialization::TkAsset>();
		
			return serializeIntoBuilder(assetBuilder, asset);
		}
		
		
		//ExtPxAsset
		template<>
		NV_INLINE bool ExtSerialization<Nv::Blast::ExtPxAsset, Nv::Blast::Serialization::ExtPxAsset::Reader, Nv::Blast::Serialization::ExtPxAsset::Builder>::serializeIntoBuilder(Nv::Blast::Serialization::ExtPxAsset::Builder& assetBuilder, const Nv::Blast::ExtPxAsset* asset)
		{
			return ExtPxAssetDTO::serialize(assetBuilder, asset);
		}
		
		template<>
		NV_INLINE Nv::Blast::ExtPxAsset* ExtSerialization<Nv::Blast::ExtPxAsset, Nv::Blast::Serialization::ExtPxAsset::Reader, Nv::Blast::Serialization::ExtPxAsset::Builder>::deserializeFromStreamReader(capnp::InputStreamMessageReader &message)
		{
			Nv::Blast::Serialization::ExtPxAsset::Reader reader = message.getRoot<Nv::Blast::Serialization::ExtPxAsset>();
		
			return ExtPxAssetDTO::deserialize(reader);
		}
		
		template<>
		NV_INLINE bool ExtSerialization<Nv::Blast::ExtPxAsset, Nv::Blast::Serialization::ExtPxAsset::Reader, Nv::Blast::Serialization::ExtPxAsset::Builder>::serializeIntoMessage(capnp::MallocMessageBuilder& message, const Nv::Blast::ExtPxAsset* asset)
		{
			Nv::Blast::Serialization::ExtPxAsset::Builder assetBuilder = message.initRoot<Nv::Blast::Serialization::ExtPxAsset>();
		
			return serializeIntoBuilder(assetBuilder, asset);
		}
	}
}
