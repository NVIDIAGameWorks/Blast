/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NvBlastExtSerializationImpl.h"
#include <memory>
#include "PxPhysicsVersion.h"
#include "PxConvexMeshGeometryDTO.h"
#include "NvBlastExtDefs.h"
#include "PxPhysics.h"
#include "NvBlastAssert.h"


// This is terrible.
physx::PxPhysics* g_Physics = nullptr;

using namespace Nv::Blast;

std::shared_ptr<physx::PxCooking> getCooking()
{
	physx::PxCookingParams cookingParams(g_Physics->getTolerancesScale());
	cookingParams.buildGPUData = true;

	std::shared_ptr<physx::PxCooking> m_Cooking = std::shared_ptr<physx::PxCooking>(PxCreateCooking(PX_PHYSICS_VERSION, g_Physics->getFoundation(), cookingParams), [=](physx::PxCooking* cooking)
	{
		cooking->release();
	});

	NVBLASTEXT_CHECK_ERROR(m_Cooking, "Error: failed to create PhysX Cooking\n", return nullptr);

	return m_Cooking;
}


extern "C"
{
	NVBLAST_API void setPhysXSDK(physx::PxPhysics* physXSDK)
	{
		g_Physics = physXSDK;
	}

	//////////////////////////////////////////////////////////////////////////
	// TkAsset
	//////////////////////////////////////////////////////////////////////////

	NVBLAST_API Nv::Blast::TkAsset* deserializeTkAsset(const unsigned char* input, uint32_t size)
	{
		return Nv::Blast::ExtSerialization<Nv::Blast::TkAsset, Nv::Blast::Serialization::TkAsset::Reader, Nv::Blast::Serialization::TkAsset::Builder>::deserialize(input, size);
	}

	NVBLAST_API Nv::Blast::TkAsset* deserializeTkAssetFromStream(std::istream &inputStream)
	{
		return Nv::Blast::ExtSerialization<Nv::Blast::TkAsset, Nv::Blast::Serialization::TkAsset::Reader, Nv::Blast::Serialization::TkAsset::Builder>::deserializeFromStream(inputStream);
	}

	NVBLAST_API bool serializeTkAssetIntoStream(const Nv::Blast::TkAsset *asset, std::ostream &outputStream)
	{
		return Nv::Blast::ExtSerialization<Nv::Blast::TkAsset, Nv::Blast::Serialization::TkAsset::Reader, Nv::Blast::Serialization::TkAsset::Builder>::serializeIntoStream(reinterpret_cast<const Nv::Blast::TkAsset *>(asset), outputStream);
	}

	NVBLAST_API bool serializeTkAssetIntoNewBuffer(const Nv::Blast::TkAsset *asset, unsigned char **outBuffer, uint32_t &outSize)
	{
		return Nv::Blast::ExtSerialization<Nv::Blast::TkAsset, Nv::Blast::Serialization::TkAsset::Reader, Nv::Blast::Serialization::TkAsset::Builder>::serializeIntoNewBuffer(reinterpret_cast<const Nv::Blast::TkAsset *>(asset), outBuffer, outSize);
	}

	NVBLAST_API bool serializeTkAssetIntoExistingBuffer(const Nv::Blast::TkAsset *asset, unsigned char *buffer, uint32_t maxSize, uint32_t &usedSize)
	{
		return Nv::Blast::ExtSerialization<Nv::Blast::TkAsset, Nv::Blast::Serialization::TkAsset::Reader, Nv::Blast::Serialization::TkAsset::Builder>::serializeIntoExistingBuffer(reinterpret_cast<const Nv::Blast::TkAsset *>(asset), buffer, maxSize, usedSize);
	}

	//////////////////////////////////////////////////////////////////////////
	// ExtPxAsset
	//////////////////////////////////////////////////////////////////////////

	NVBLAST_API Nv::Blast::ExtPxAsset* deserializeExtPxAsset(const unsigned char* input, uint32_t size)
	{
		NVBLAST_ASSERT(g_Physics != nullptr);

		return Nv::Blast::ExtSerialization<Nv::Blast::ExtPxAsset, Nv::Blast::Serialization::ExtPxAsset::Reader, Nv::Blast::Serialization::ExtPxAsset::Builder>::deserialize(input, size);
	}

	NVBLAST_API Nv::Blast::ExtPxAsset* deserializeExtPxAssetFromStream(std::istream &inputStream)
	{
		NVBLAST_ASSERT(g_Physics != nullptr);

		return Nv::Blast::ExtSerialization<Nv::Blast::ExtPxAsset, Nv::Blast::Serialization::ExtPxAsset::Reader, Nv::Blast::Serialization::ExtPxAsset::Builder>::deserializeFromStream(inputStream);
	}

	NVBLAST_API bool serializeExtPxAssetIntoStream(const Nv::Blast::ExtPxAsset *asset, std::ostream &outputStream)
	{
		NVBLAST_ASSERT(g_Physics != nullptr);

		auto cooking = getCooking();

		PxConvexMeshGeometryDTO::Cooking = cooking.get();
		PxConvexMeshGeometryDTO::Physics = g_Physics;

		return Nv::Blast::ExtSerialization<Nv::Blast::ExtPxAsset, Nv::Blast::Serialization::ExtPxAsset::Reader, Nv::Blast::Serialization::ExtPxAsset::Builder>::serializeIntoStream(reinterpret_cast<const Nv::Blast::ExtPxAsset *>(asset), outputStream);
	}

	NVBLAST_API bool serializeExtPxAssetIntoNewBuffer(const Nv::Blast::ExtPxAsset *asset, unsigned char **outBuffer, uint32_t &outSize)
	{
		NVBLAST_ASSERT(g_Physics != nullptr);

		auto cooking = getCooking();

		PxConvexMeshGeometryDTO::Cooking = cooking.get();
		PxConvexMeshGeometryDTO::Physics = g_Physics;

		return Nv::Blast::ExtSerialization<Nv::Blast::ExtPxAsset, Nv::Blast::Serialization::ExtPxAsset::Reader, Nv::Blast::Serialization::ExtPxAsset::Builder>::serializeIntoNewBuffer(reinterpret_cast<const Nv::Blast::ExtPxAsset *>(asset), outBuffer, outSize);
	}

	NVBLAST_API bool serializeExtPxAssetIntoExistingBuffer(const Nv::Blast::ExtPxAsset *asset, unsigned char *buffer, uint32_t maxSize, uint32_t &usedSize)
	{
		NVBLAST_ASSERT(g_Physics != nullptr);

		auto cooking = getCooking();

		PxConvexMeshGeometryDTO::Cooking = cooking.get();
		PxConvexMeshGeometryDTO::Physics = g_Physics;

		return Nv::Blast::ExtSerialization<Nv::Blast::ExtPxAsset, Nv::Blast::Serialization::ExtPxAsset::Reader, Nv::Blast::Serialization::ExtPxAsset::Builder>::serializeIntoExistingBuffer(reinterpret_cast<const Nv::Blast::ExtPxAsset *>(asset), buffer, maxSize, usedSize);
	}


}

