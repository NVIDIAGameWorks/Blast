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


#include "TkAssetDTO.h"
#include "AssetDTO.h"
#include "TkAssetJointDescDTO.h"
#include <vector>
#include "NvBlastTkFramework.h"
#include "NvBlastGlobals.h"


namespace Nv
{
namespace Blast
{

extern TkFramework* sExtTkSerializerFramework;


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

	const uint32_t jointDescCount = reader.getJointDescs().size();
	jointDescs.resize(jointDescCount);
	auto readerJointDescs = reader.getJointDescs();
	for (uint32_t i = 0; i < jointDescCount; i++)
	{
		TkAssetJointDescDTO::deserializeInto(readerJointDescs[i], &jointDescs[i]);
	}

	// Make sure to set ownsAsset to true - this is serialization and no one else owns it.
	Nv::Blast::TkAsset* asset = NvBlastTkFrameworkGet()->createAsset(assetLL, jointDescs.data(), jointDescCount, true);

	return asset;
}


bool TkAssetDTO::deserializeInto(Nv::Blast::Serialization::TkAsset::Reader reader, Nv::Blast::TkAsset * poco)
{
	NV_UNUSED(reader);
	poco = nullptr;
	// NOTE: Because of the way TkAsset is currently structured, this won't work.
	return false;
}

}	// namespace Blast
}	// namespace Nv
