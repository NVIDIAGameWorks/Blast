#include "NvBlastExtSerialization.h"
#include "BlastSerialization.h"
#include <memory>
#include "PxPhysicsVersion.h"
#include "PxConvexMeshGeometryDTO.h"
#include "NvBlastExtDefs.h"


// This is terrible.
physx::PxPhysics* g_Physics = nullptr;


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

	NVBLAST_API NvBlastAsset* deserializeAsset(const unsigned char* input, uint32_t size)
	{
		return Nv::Blast::BlastSerialization<Nv::Blast::Asset, Nv::Blast::Serialization::Asset::Reader, Nv::Blast::Serialization::Asset::Builder>::deserialize(input, size);
	}

	NVBLAST_API NvBlastAsset* deserializeAssetFromStream(std::istream &inputStream)
	{
		return Nv::Blast::BlastSerialization<Nv::Blast::Asset, Nv::Blast::Serialization::Asset::Reader, Nv::Blast::Serialization::Asset::Builder>::deserializeFromStream(inputStream);
	}
	
	NVBLAST_API bool serializeAssetIntoStream(const NvBlastAsset *asset, std::ostream &outputStream)
	{
		return Nv::Blast::BlastSerialization<Nv::Blast::Asset, Nv::Blast::Serialization::Asset::Reader, Nv::Blast::Serialization::Asset::Builder>::serializeIntoStream(reinterpret_cast<const Nv::Blast::Asset *>(asset), outputStream);
	}

	NVBLAST_API bool serializeAssetIntoNewBuffer(const NvBlastAsset *asset, unsigned char **outBuffer, uint32_t &outSize)
	{
		return Nv::Blast::BlastSerialization<Nv::Blast::Asset, Nv::Blast::Serialization::Asset::Reader, Nv::Blast::Serialization::Asset::Builder>::serializeIntoNewBuffer(reinterpret_cast<const Nv::Blast::Asset *>(asset), outBuffer, outSize);
	}

	NVBLAST_API bool serializeAssetIntoExistingBuffer(const NvBlastAsset *asset, unsigned char *buffer, uint32_t maxSize, uint32_t &usedSize)
	{
		return Nv::Blast::BlastSerialization<Nv::Blast::Asset, Nv::Blast::Serialization::Asset::Reader, Nv::Blast::Serialization::Asset::Builder>::serializeIntoExistingBuffer(reinterpret_cast<const Nv::Blast::Asset *>(asset), buffer, maxSize, usedSize);
	}

	//////////////////////////////////////////////////////////////////////////
	// TkAsset
	//////////////////////////////////////////////////////////////////////////

	NVBLAST_API Nv::Blast::TkAsset* deserializeTkAsset(const unsigned char* input, uint32_t size)
	{
		return Nv::Blast::BlastSerialization<Nv::Blast::TkAsset, Nv::Blast::Serialization::TkAsset::Reader, Nv::Blast::Serialization::TkAsset::Builder>::deserialize(input, size);
	}

	NVBLAST_API Nv::Blast::TkAsset* deserializeTkAssetFromStream(std::istream &inputStream)
	{
		return Nv::Blast::BlastSerialization<Nv::Blast::TkAsset, Nv::Blast::Serialization::TkAsset::Reader, Nv::Blast::Serialization::TkAsset::Builder>::deserializeFromStream(inputStream);
	}

	NVBLAST_API bool serializeTkAssetIntoStream(const Nv::Blast::TkAsset *asset, std::ostream &outputStream)
	{
		return Nv::Blast::BlastSerialization<Nv::Blast::TkAsset, Nv::Blast::Serialization::TkAsset::Reader, Nv::Blast::Serialization::TkAsset::Builder>::serializeIntoStream(reinterpret_cast<const Nv::Blast::TkAsset *>(asset), outputStream);
	}

	NVBLAST_API bool serializeTkAssetIntoNewBuffer(const Nv::Blast::TkAsset *asset, unsigned char **outBuffer, uint32_t &outSize)
	{
		return Nv::Blast::BlastSerialization<Nv::Blast::TkAsset, Nv::Blast::Serialization::TkAsset::Reader, Nv::Blast::Serialization::TkAsset::Builder>::serializeIntoNewBuffer(reinterpret_cast<const Nv::Blast::TkAsset *>(asset), outBuffer, outSize);
	}

	NVBLAST_API bool serializeTkAssetIntoExistingBuffer(const Nv::Blast::TkAsset *asset, unsigned char *buffer, uint32_t maxSize, uint32_t &usedSize)
	{
		return Nv::Blast::BlastSerialization<Nv::Blast::TkAsset, Nv::Blast::Serialization::TkAsset::Reader, Nv::Blast::Serialization::TkAsset::Builder>::serializeIntoExistingBuffer(reinterpret_cast<const Nv::Blast::TkAsset *>(asset), buffer, maxSize, usedSize);
	}

	//////////////////////////////////////////////////////////////////////////
	// ExtPxAsset
	//////////////////////////////////////////////////////////////////////////

	NVBLAST_API Nv::Blast::ExtPxAsset* deserializeExtPxAsset(const unsigned char* input, uint32_t size)
	{
		NVBLAST_ASSERT(g_Physics != nullptr);

		return Nv::Blast::BlastSerialization<Nv::Blast::ExtPxAsset, Nv::Blast::Serialization::ExtPxAsset::Reader, Nv::Blast::Serialization::ExtPxAsset::Builder>::deserialize(input, size);
	}

	NVBLAST_API Nv::Blast::ExtPxAsset* deserializeExtPxAssetFromStream(std::istream &inputStream)
	{
		NVBLAST_ASSERT(g_Physics != nullptr);

		return Nv::Blast::BlastSerialization<Nv::Blast::ExtPxAsset, Nv::Blast::Serialization::ExtPxAsset::Reader, Nv::Blast::Serialization::ExtPxAsset::Builder>::deserializeFromStream(inputStream);
	}

	NVBLAST_API bool serializeExtPxAssetIntoStream(const Nv::Blast::ExtPxAsset *asset, std::ostream &outputStream)
	{
		NVBLAST_ASSERT(g_Physics != nullptr);

		auto cooking = getCooking();

		PxConvexMeshGeometryDTO::Cooking = cooking.get();
		PxConvexMeshGeometryDTO::Physics = g_Physics;

		return Nv::Blast::BlastSerialization<Nv::Blast::ExtPxAsset, Nv::Blast::Serialization::ExtPxAsset::Reader, Nv::Blast::Serialization::ExtPxAsset::Builder>::serializeIntoStream(reinterpret_cast<const Nv::Blast::ExtPxAsset *>(asset), outputStream);
	}

	NVBLAST_API bool serializeExtPxAssetIntoNewBuffer(const Nv::Blast::ExtPxAsset *asset, unsigned char **outBuffer, uint32_t &outSize)
	{
		NVBLAST_ASSERT(g_Physics != nullptr);

		auto cooking = getCooking();

		PxConvexMeshGeometryDTO::Cooking = cooking.get();
		PxConvexMeshGeometryDTO::Physics = g_Physics;

		return Nv::Blast::BlastSerialization<Nv::Blast::ExtPxAsset, Nv::Blast::Serialization::ExtPxAsset::Reader, Nv::Blast::Serialization::ExtPxAsset::Builder>::serializeIntoNewBuffer(reinterpret_cast<const Nv::Blast::ExtPxAsset *>(asset), outBuffer, outSize);
	}

	NVBLAST_API bool serializeExtPxAssetIntoExistingBuffer(const Nv::Blast::ExtPxAsset *asset, unsigned char *buffer, uint32_t maxSize, uint32_t &usedSize)
	{
		NVBLAST_ASSERT(g_Physics != nullptr);

		auto cooking = getCooking();

		PxConvexMeshGeometryDTO::Cooking = cooking.get();
		PxConvexMeshGeometryDTO::Physics = g_Physics;

		return Nv::Blast::BlastSerialization<Nv::Blast::ExtPxAsset, Nv::Blast::Serialization::ExtPxAsset::Reader, Nv::Blast::Serialization::ExtPxAsset::Builder>::serializeIntoExistingBuffer(reinterpret_cast<const Nv::Blast::ExtPxAsset *>(asset), buffer, maxSize, usedSize);
	}


}

