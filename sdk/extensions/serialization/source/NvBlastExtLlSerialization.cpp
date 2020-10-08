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
#include "NvBlastExtLlSerialization.h"
#include "NvBlastExtLlSerializerCAPN.h"


namespace Nv
{
namespace Blast
{

class ExtLlSerializerAsset_CPNB : public ExtSerializer
{
public:
	ExtSerializerBoilerplate("LLAsset_CPNB", "Blast low-level asset (NvBlastAsset) serialization using Cap'n Proto binary format.", LlObjectTypeID::Asset, ExtSerialization::EncodingID::CapnProtoBinary);
	ExtSerializerDefaultFactoryAndRelease(ExtLlSerializerAsset_CPNB);

	virtual void* deserializeFromBuffer(const void* buffer, uint64_t size) override
	{
		return ExtSerializationCAPN<Asset, Serialization::Asset::Reader, Serialization::Asset::Builder>::deserializeFromBuffer(reinterpret_cast<const unsigned char*>(buffer), size);
	}

	virtual uint64_t serializeIntoBuffer(void*& buffer, ExtSerialization::BufferProvider& bufferProvider, const void* object, uint64_t offset = 0) override
	{
		uint64_t usedSize;
		if (!ExtSerializationCAPN<Asset, Serialization::Asset::Reader, Serialization::Asset::Builder>::serializeIntoBuffer(reinterpret_cast<const Asset*>(object),
			reinterpret_cast<unsigned char*&>(buffer), usedSize, &bufferProvider, offset))
		{
			return 0;
		}
		return usedSize;
	}
};


class ExtLlSerializerObject_RAW : public ExtSerializer
{
public:
	virtual void* deserializeFromBuffer(const void* buffer, uint64_t size) override
	{
		const NvBlastDataBlock* block = reinterpret_cast<const NvBlastDataBlock*>(buffer);
		if (static_cast<uint64_t>(block->size) > size)
		{
			return nullptr;
		}
		void* llobject = NVBLAST_ALLOC(block->size);
		return memcpy(llobject, block, block->size);
	}

	virtual uint64_t serializeIntoBuffer(void*& buffer, ExtSerialization::BufferProvider& bufferProvider, const void* object, uint64_t offset = 0) override
	{
		const NvBlastDataBlock* block = reinterpret_cast<const NvBlastDataBlock*>(object);
		const uint64_t size = block->size + offset;
		buffer = bufferProvider.requestBuffer(size);
		if (buffer == nullptr)
		{
			return 0;
		}
		memcpy(static_cast<char*>(buffer) + offset, object, block->size);
		return size;
	}
};


class ExtLlSerializerAsset_RAW : public ExtLlSerializerObject_RAW
{
public:
	ExtSerializerBoilerplate("LLAsset_RAW", "Blast low-level asset (NvBlastAsset) serialization using raw memory format.", LlObjectTypeID::Asset, ExtSerialization::EncodingID::RawBinary);
	ExtSerializerDefaultFactoryAndRelease(ExtLlSerializerAsset_RAW);
};


class ExtLlSerializerFamily_RAW : public ExtLlSerializerObject_RAW
{
public:
	ExtSerializerBoilerplate("LLFamily_RAW", "Blast low-level family (NvBlastFamily) serialization using raw memory format.", LlObjectTypeID::Family, ExtSerialization::EncodingID::RawBinary);
	ExtSerializerDefaultFactoryAndRelease(ExtLlSerializerFamily_RAW);
};

}	// namespace Blast
}	// namespace Nv


///////////////////////////////////////


size_t NvBlastExtLlSerializerLoadSet(Nv::Blast::ExtSerialization& serialization)
{
	Nv::Blast::ExtSerializer* (*factories[])() =
	{
		Nv::Blast::ExtLlSerializerAsset_CPNB::create,
		Nv::Blast::ExtLlSerializerAsset_RAW::create,
		Nv::Blast::ExtLlSerializerFamily_RAW::create
	};

	return Nv::Blast::ExtSerializationLoadSet(static_cast<Nv::Blast::ExtSerializationInternal&>(serialization), factories);
}


uint64_t NvBlastExtSerializationSerializeAssetIntoBuffer(void*& buffer, Nv::Blast::ExtSerialization& serialization, const NvBlastAsset* asset)
{
	return serialization.serializeIntoBuffer(buffer, asset, Nv::Blast::LlObjectTypeID::Asset);
}


uint64_t NvBlastExtSerializationSerializeFamilyIntoBuffer(void*& buffer, Nv::Blast::ExtSerialization& serialization, const NvBlastFamily* family)
{
	return serialization.serializeIntoBuffer(buffer, family, Nv::Blast::LlObjectTypeID::Family);
}
