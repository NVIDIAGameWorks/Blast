/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NvBlastExtDataConverter.h"
#include "NvBlastExtBinaryBlockConverter.h"

#include <iostream>

// asset converters
#include "NvBlastExtAssetBlockVersionConverter_v0_v1.h"


namespace Nv
{
namespace Blast
{

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Asset Block Converter
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

NV_INLINE std::vector<BinaryBlockConverter::VersionConverterPtr> getAssetConverters()
{
	/**
		+==========================================+
		|  HINT: ADD NEW VERSION CONVERTERS THERE  |
		+==========================================+
	*/
	BinaryBlockConverter::VersionConverterPtr converters[] = 
	{
		std::make_shared<NvBlastAssetBlockVersionConverter_v0_v1>()
	};

	return std::vector<BinaryBlockConverter::VersionConverterPtr>(converters, converters + sizeof(converters) / sizeof(converters[0]));
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Family Converter
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

NV_INLINE std::vector<BinaryBlockConverter::VersionConverterPtr> getFamilyConverters()
{
	/**
		+==========================================+
		|  HINT: ADD NEW VERSION CONVERTERS THERE  |
		+==========================================+
	*/
	BinaryBlockConverter::VersionConverterPtr converters[] = 
	{
		nullptr //std::make_shared<NvBlastFamilyVersionConverter_v0_v1>()
	};

	return std::vector<BinaryBlockConverter::VersionConverterPtr>(converters, converters + sizeof(converters) / sizeof(converters[0]));
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Generic Block Converter
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool convertDataBlock(std::vector<char>& outBlock, const std::vector<char>& inBlock, uint32_t* outBlockVersion)
{
	// Pick header to determine dataType and version
	if (inBlock.size() < sizeof(NvBlastDataBlock))
	{
		std::cerr << "Conversion failed: invalid block, passed block is too small." << std::endl;
		return false;
	}
	const NvBlastDataBlock* dataBlock = reinterpret_cast<const NvBlastDataBlock*>(inBlock.data());
		
	// Select appropriate converters and version based on dataType
	std::vector<BinaryBlockConverter::VersionConverterPtr> converters;
	uint32_t version;
	switch (dataBlock->dataType)
	{
	case NvBlastDataBlock::AssetDataBlock:
		std::cout << "Input block dataType: NvBlastDataBlock::Asset" << std::endl;
		converters = getAssetConverters();
		version = (outBlockVersion == nullptr ? static_cast<uint32_t>(NvBlastAssetDataFormat::Current) : *outBlockVersion);
		break;
	case NvBlastDataBlock::FamilyDataBlock:
		std::cout << "Input block dataType: NvBlastDataBlock::Family" << std::endl;
		converters = getFamilyConverters();
		version = (outBlockVersion == nullptr ? static_cast<uint32_t>(NvBlastFamilyDataFormat::Current) : *outBlockVersion);
		break;
	default:
		std::cerr << "Conversion failed: unsupported dataType: " << dataBlock->dataType << std::endl;
		return false;
	}

	return BinaryBlockConverter::convertBinaryBlock(outBlock, converters, inBlock, version, dataBlock->formatVersion);
}

} // namespace Blast
} // namespace Nv
