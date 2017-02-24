/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "TkAssetDTO.h"
#include "AssetDTO.h"
#include "TkAssetJointDescDTO.h"
#include <vector>
#include "NvBlastTkFramework.h"



namespace Nv
{
	namespace Blast
	{
		bool TkAssetDTO::serialize(Nv::Blast::Serialization::TkAsset::Builder builder, const Nv::Blast::TkAsset * poco)
		{
			const Asset* assetLL = reinterpret_cast<const Nv::Blast::Asset*>(poco->getAssetLL());
		
			Nv::Blast::AssetDTO::serialize(builder.getAssetLL(), assetLL);
		
			uint32_t jointDescCount = poco->getJointDescCount();
		
			capnp::List<Nv::Blast::Serialization::TkAssetJointDesc>::Builder jointDescs = builder.initJointDescs(jointDescCount);
		
			for (uint32_t i = 0; i < jointDescCount; i++)
			{
				TkAssetJointDescDTO::serialize(jointDescs[i], &poco->getJointDescs()[i]);
			}
		
			return true;
		}
		
		Nv::Blast::TkAsset* TkAssetDTO::deserialize(Nv::Blast::Serialization::TkAsset::Reader reader)
		{
			const NvBlastAsset* assetLL = reinterpret_cast<const NvBlastAsset*>(AssetDTO::deserialize(reader.getAssetLL()));
			
			std::vector<Nv::Blast::TkAssetJointDesc> jointDescs;
			jointDescs.resize(reader.getJointDescs().size());
		
			for (uint32_t i = 0; i < jointDescs.size(); i++)
			{
				TkAssetJointDescDTO::deserializeInto(reader.getJointDescs()[i], &jointDescs[i]);
			}
		
			// Make sure to set ownsAsset to true - this is serialization and no one else owns it.
			Nv::Blast::TkAsset* asset = NvBlastTkFrameworkGet()->createAsset(assetLL, jointDescs.data(), jointDescs.size(), true);
		
			return asset;
		}
		
		bool TkAssetDTO::deserializeInto(Nv::Blast::Serialization::TkAsset::Reader reader, Nv::Blast::TkAsset * poco)
		{
			reader = reader;
			poco = nullptr;
			// NOTE: Because of the way TkAsset is currently structured, this won't work.
			return false;
		}
	}
}
