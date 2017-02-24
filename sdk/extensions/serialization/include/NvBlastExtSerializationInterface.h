/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

/*
	Include this file to get the C interface to serialization for all asset types (LL, Tk and Ext)
*/
#pragma once
#include <ostream>
#include "NvBlastTkAsset.h"
#include "NvBlastExtPxAsset.h"

#include "NvBlastExtSerializationLLInterface.h"

namespace physx
{
	class PxPhysics;
}

NVBLAST_API void setPhysXSDK(physx::PxPhysics* physXSDK);

NVBLAST_API Nv::Blast::TkAsset* deserializeTkAsset(const unsigned char* input, uint32_t size);
NVBLAST_API Nv::Blast::TkAsset* deserializeTkAssetFromStream(std::istream &inputStream);
NVBLAST_API bool serializeTkAssetIntoStream(const Nv::Blast::TkAsset *asset, std::ostream &outputStream);
NVBLAST_API bool serializeTkAssetIntoNewBuffer(const Nv::Blast::TkAsset *asset, unsigned char **outBuffer, uint32_t &outSize);
NVBLAST_API bool serializeTkAssetIntoExistingBuffer(const Nv::Blast::TkAsset *asset, unsigned char *buffer, uint32_t maxSize, uint32_t &usedSize);

NVBLAST_API Nv::Blast::ExtPxAsset* deserializeExtPxAsset(const unsigned char* input, uint32_t size);
NVBLAST_API Nv::Blast::ExtPxAsset* deserializeExtPxAssetFromStream(std::istream &inputStream);
NVBLAST_API bool serializeExtPxAssetIntoStream(const Nv::Blast::ExtPxAsset *asset, std::ostream &outputStream);
NVBLAST_API bool serializeExtPxAssetIntoNewBuffer(const Nv::Blast::ExtPxAsset *asset, unsigned char **outBuffer, uint32_t &outSize);
NVBLAST_API bool serializeExtPxAssetIntoExistingBuffer(const Nv::Blast::ExtPxAsset *asset, unsigned char *buffer, uint32_t maxSize, uint32_t &usedSize);
