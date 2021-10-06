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


#pragma once

#include "NvBlastExtSerializationCAPN.h"
#include "NvBlastExtPxAsset.h"
#include "ExtPxAssetDTO.h"


/**
Specializations of ExtSerializationCAPN for BlastExtPx
*/

namespace Nv
{
namespace Blast
{

//// Nv::Blast::ExtPxAsset ////

template<>
NV_INLINE bool ExtSerializationCAPN<ExtPxAsset, Serialization::ExtPxAsset::Reader, Serialization::ExtPxAsset::Builder>::serializeIntoBuilder(Serialization::ExtPxAsset::Builder& assetBuilder, const ExtPxAsset* asset)
{
	return ExtPxAssetDTO::serialize(assetBuilder, asset);
}

template<>
NV_INLINE ExtPxAsset* ExtSerializationCAPN<ExtPxAsset, Serialization::ExtPxAsset::Reader, Serialization::ExtPxAsset::Builder>::deserializeFromStreamReader(capnp::InputStreamMessageReader &message)
{
	Serialization::ExtPxAsset::Reader reader = message.getRoot<Serialization::ExtPxAsset>();

	return ExtPxAssetDTO::deserialize(reader);
}

template<>
NV_INLINE bool ExtSerializationCAPN<ExtPxAsset, Serialization::ExtPxAsset::Reader, Serialization::ExtPxAsset::Builder>::serializeIntoMessage(capnp::MallocMessageBuilder& message, const ExtPxAsset* asset)
{
	Serialization::ExtPxAsset::Builder assetBuilder = message.initRoot<Serialization::ExtPxAsset>();

	return serializeIntoBuilder(assetBuilder, asset);
}

}	// namespace Blast
}	// namespace Nv
