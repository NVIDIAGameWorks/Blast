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


#include "NvBlastExtSerializationInternal.h"
#include "NvBlastExtTkSerialization.h"
#include "NvBlastExtTkSerializerCAPN.h"
#include "NvBlastExtTkSerializerRAW.h"


namespace Nv
{
namespace Blast
{

TkFramework* sExtTkSerializerFramework = nullptr;


class ExtTkSerializerAsset_CPNB : public ExtSerializer
{
public:
	ExtSerializerBoilerplate("TkAsset_CPNB", "Blast high-level asset (Nv::Blast::TkAsset) serialization using Cap'n Proto binary format.", TkObjectTypeID::Asset, ExtSerialization::EncodingID::CapnProtoBinary);
	ExtSerializerDefaultFactoryAndRelease(ExtTkSerializerAsset_CPNB);

	virtual void* deserializeFromBuffer(const void* buffer, uint64_t size) override
	{
		return ExtSerializationCAPN<TkAsset, Serialization::TkAsset::Reader, Serialization::TkAsset::Builder>::deserializeFromBuffer(reinterpret_cast<const unsigned char*>(buffer), size);
	}

	virtual uint64_t serializeIntoBuffer(void*& buffer, ExtSerialization::BufferProvider& bufferProvider, const void* object, uint64_t offset = 0) override
	{
		uint64_t usedSize;
		if (!ExtSerializationCAPN<TkAsset, Serialization::TkAsset::Reader, Serialization::TkAsset::Builder>::serializeIntoBuffer(reinterpret_cast<const TkAsset*>(object),
			reinterpret_cast<unsigned char*&>(buffer), usedSize, &bufferProvider, offset))
		{
			return 0;
		}
		return usedSize;
	}
};


class ExTkSerializerAsset_RAW : public ExtSerializer
{
public:
	ExtSerializerBoilerplate("TkAsset_RAW", "Blast high-level asset (Nv::Blast::TkAsset) serialization using raw memory format.", TkObjectTypeID::Asset, ExtSerialization::EncodingID::RawBinary);
	ExtSerializerDefaultFactoryAndRelease(ExTkSerializerAsset_RAW);
	ExtSerializerReadOnly(ExTkSerializerAsset_RAW);

	virtual void* deserializeFromBuffer(const void* buffer, uint64_t size) override
	{
		ExtIStream stream(buffer, size);
		return deserializeTkAsset(stream, *sExtTkSerializerFramework);
	}
};

}	// namespace Blast
}	// namespace Nv


///////////////////////////////////////


size_t NvBlastExtTkSerializerLoadSet(Nv::Blast::TkFramework& framework, Nv::Blast::ExtSerialization& serialization)
{
	Nv::Blast::sExtTkSerializerFramework = &framework;

	Nv::Blast::ExtSerializer* (*factories[])() =
	{
		Nv::Blast::ExtTkSerializerAsset_CPNB::create,
		Nv::Blast::ExTkSerializerAsset_RAW::create
	};

	return Nv::Blast::ExtSerializationLoadSet(static_cast<Nv::Blast::ExtSerializationInternal&>(serialization), factories);
}


uint64_t NvBlastExtSerializationSerializeTkAssetIntoBuffer(void*& buffer, Nv::Blast::ExtSerialization& serialization, const Nv::Blast::TkAsset* asset)
{
	return serialization.serializeIntoBuffer(buffer, asset, Nv::Blast::TkObjectTypeID::Asset);
}
