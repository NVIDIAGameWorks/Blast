/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NvBlastExtSerializationLLInterface.h"
#include "NvBlastExtSerializationLLImpl.h"
#include <memory>
#include "NvBlastExtSerialization.h"
#include "NvBlastAsset.h"
#include <iostream>
#include "NvBlastExtGlobals.h"


NvBlastExtAlloc gAlloc = nullptr;
NvBlastLog gLog = nullptr;

extern "C"
{
	NVBLAST_API void setAllocator(NvBlastExtAlloc alloc)
	{
		gAlloc = alloc;
	}

	NVBLAST_API void setLog(NvBlastLog log)
	{
		gLog = log;
	}

	NVBLAST_API NvBlastAsset* deserializeAsset(const unsigned char* input, uint32_t size)
	{
#if defined(BLAST_LL_ALLOC)
		if (gAlloc == nullptr || gLog == nullptr)
		{
			std::cerr << "Must set allocator and log when using low level serialization library. See setAllocator() and setLog() functions." << std::endl;
			return nullptr;
		}
#endif

		return Nv::Blast::ExtSerialization<Nv::Blast::Asset, Nv::Blast::Serialization::Asset::Reader, Nv::Blast::Serialization::Asset::Builder>::deserialize(input, size);
	}

	NVBLAST_API NvBlastAsset* deserializeAssetFromStream(std::istream &inputStream)
	{
#if defined(BLAST_LL_ALLOC)
		if (gAlloc == nullptr || gLog == nullptr)
		{
			std::cerr << "Must set allocator and log when using low level serialization library. See setAllocator() and setLog() functions." << std::endl;
			return nullptr;
		}
#endif

		return Nv::Blast::ExtSerialization<Nv::Blast::Asset, Nv::Blast::Serialization::Asset::Reader, Nv::Blast::Serialization::Asset::Builder>::deserializeFromStream(inputStream);
	}
	
	NVBLAST_API bool serializeAssetIntoStream(const NvBlastAsset *asset, std::ostream &outputStream)
	{
#if defined(BLAST_LL_ALLOC)
		if (gAlloc == nullptr || gLog == nullptr)
		{
			std::cerr << "Must set allocator and log when using low level serialization library. See setAllocator() and setLog() functions." << std::endl;
			return false;
		}
#endif

		return Nv::Blast::ExtSerialization<Nv::Blast::Asset, Nv::Blast::Serialization::Asset::Reader, Nv::Blast::Serialization::Asset::Builder>::serializeIntoStream(reinterpret_cast<const Nv::Blast::Asset *>(asset), outputStream);
	}

	NVBLAST_API bool serializeAssetIntoNewBuffer(const NvBlastAsset *asset, unsigned char **outBuffer, uint32_t &outSize)
	{
#if defined(BLAST_LL_ALLOC)
		if (gAlloc == nullptr || gLog == nullptr)
		{
			std::cerr << "Must set allocator and log when using low level serialization library. See setAllocator() and setLog() functions." << std::endl;
			return false;
		}
#endif

		return Nv::Blast::ExtSerialization<Nv::Blast::Asset, Nv::Blast::Serialization::Asset::Reader, Nv::Blast::Serialization::Asset::Builder>::serializeIntoNewBuffer(reinterpret_cast<const Nv::Blast::Asset *>(asset), outBuffer, outSize);
	}

	NVBLAST_API bool serializeAssetIntoExistingBuffer(const NvBlastAsset *asset, unsigned char *buffer, uint32_t maxSize, uint32_t &usedSize)
	{
#if defined(BLAST_LL_ALLOC)
		if (gAlloc == nullptr || gLog == nullptr)
		{
			std::cerr << "Must set allocator and log when using low level serialization library. See setAllocator() and setLog() functions." << std::endl;
			return false;
		}
#endif

		return Nv::Blast::ExtSerialization<Nv::Blast::Asset, Nv::Blast::Serialization::Asset::Reader, Nv::Blast::Serialization::Asset::Builder>::serializeIntoExistingBuffer(reinterpret_cast<const Nv::Blast::Asset *>(asset), buffer, maxSize, usedSize);
	}

}

