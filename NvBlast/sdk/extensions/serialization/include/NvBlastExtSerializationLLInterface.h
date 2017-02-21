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
Include this file to access the C API for serialization, for Low Level assets only.

For Serialization of Tk and Ext assets, include only NvBlastExtSerializationInterface.h, which will include this file as well.

*/
#pragma once
#include <ostream>
#include "NvBlastPreprocessor.h"
#include "NvBlastTypes.h"

#include "NvBlastExtGlobals.h"

/*
	Set a global NvBlastAlloc signature allocation function that the deserialization will use when required.

	NOTE: This will NOT be used when using the combined serialization library, as it will use the TkFramework's allocation and logging
*/
NVBLAST_API void setAllocator(NvBlastExtAlloc alloc);

/*
	Set a global NvBlastLog signature allocation function that the library will use when required.

	NOTE: This will NOT be used when using the combined serialization library, as it will use the TkFramework's allocation and logging
*/
NVBLAST_API void setLog(NvBlastLog log);


NVBLAST_API NvBlastAsset* deserializeAsset(const unsigned char* input, uint32_t size);
NVBLAST_API NvBlastAsset* deserializeAssetFromStream(std::istream &inputStream);
NVBLAST_API bool serializeAssetIntoStream(const NvBlastAsset *asset, std::ostream &outputStream);
NVBLAST_API bool serializeAssetIntoNewBuffer(const NvBlastAsset *asset, unsigned char **outBuffer, uint32_t &outSize);
NVBLAST_API bool serializeAssetIntoExistingBuffer(const NvBlastAsset *asset, unsigned char *buffer, uint32_t maxSize, uint32_t &usedSize);
