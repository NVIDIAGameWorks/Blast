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
#include "NvBlastExtPxSerialization.h"
#include "NvBlastExtPxSerializerCAPN.h"
#include "NvBlastExtPxSerializerRAW.h"


namespace Nv
{
namespace Blast
{

TkFramework*		sExtPxSerializerFramework = nullptr;
physx::PxPhysics*	sExtPxSerializerPhysics = nullptr;
physx::PxCooking*	sExtPxSerializerCooking = nullptr;


class ExtPxSerializerAsset_CPNB : public ExtSerializer
{
public:
	ExtSerializerBoilerplate("ExtPxAsset_CPNB", "Blast PhysX extension asset (Nv::Blast::ExtPxAsset) serialization using Cap'n Proto binary format.", ExtPxObjectTypeID::Asset, ExtSerialization::EncodingID::CapnProtoBinary);
	ExtSerializerDefaultFactoryAndRelease(ExtPxSerializerAsset_CPNB);

	virtual void* deserializeFromBuffer(const void* buffer, uint64_t size) override
	{
		return ExtSerializationCAPN<ExtPxAsset, Serialization::ExtPxAsset::Reader, Serialization::ExtPxAsset::Builder>::deserializeFromBuffer(reinterpret_cast<const unsigned char*>(buffer), size);
	}

	virtual uint64_t serializeIntoBuffer(void*& buffer, ExtSerialization::BufferProvider& bufferProvider, const void* object, uint64_t offset = 0) override
	{
		uint64_t usedSize;
		if (!ExtSerializationCAPN<ExtPxAsset, Serialization::ExtPxAsset::Reader, Serialization::ExtPxAsset::Builder>::serializeIntoBuffer(reinterpret_cast<const ExtPxAsset*>(object),
			reinterpret_cast<unsigned char*&>(buffer), usedSize, &bufferProvider, offset))
		{
			return 0;
		}
		return usedSize;
	}
};


class ExtPxSerializerAsset_RAW : public ExtSerializer
{
public:
	ExtSerializerBoilerplate("ExtPxAsset_RAW", "Blast PhysX extension asset (Nv::Blast::TkAsset) serialization using raw memory format.", ExtPxObjectTypeID::Asset, ExtSerialization::EncodingID::RawBinary);
	ExtSerializerDefaultFactoryAndRelease(ExtPxSerializerAsset_RAW);
	ExtSerializerReadOnly(ExtPxSerializerAsset_RAW);

	virtual void* deserializeFromBuffer(const void* buffer, uint64_t size) override
	{
		ExtIStream stream(buffer, size);
		return deserializeExtPxAsset(stream, *sExtPxSerializerFramework, *sExtPxSerializerPhysics);
	}
};

}	// namespace Blast
}	// namespace Nv


///////////////////////////////////////


size_t NvBlastExtPxSerializerLoadSet(Nv::Blast::TkFramework& framework, physx::PxPhysics& physics, physx::PxCooking& cooking, Nv::Blast::ExtSerialization& serialization)
{
	Nv::Blast::sExtPxSerializerFramework = &framework;
	Nv::Blast::sExtPxSerializerPhysics = &physics;
	Nv::Blast::sExtPxSerializerCooking = &cooking;

	Nv::Blast::ExtSerializer* (*factories[])() =
	{
		Nv::Blast::ExtPxSerializerAsset_CPNB::create,
		Nv::Blast::ExtPxSerializerAsset_RAW::create
	};

	return Nv::Blast::ExtSerializationLoadSet(static_cast<Nv::Blast::ExtSerializationInternal&>(serialization), factories);
}


uint64_t NvBlastExtSerializationSerializeExtPxAssetIntoBuffer(void*& buffer, Nv::Blast::ExtSerialization& serialization, const Nv::Blast::ExtPxAsset* asset)
{
	return serialization.serializeIntoBuffer(buffer, asset, Nv::Blast::ExtPxObjectTypeID::Asset);
}
