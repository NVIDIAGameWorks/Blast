/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTEXTDATACONVERTER_H
#define NVBLASTEXTDATACONVERTER_H


#include "NvBlast.h"
#include <vector>

namespace Nv
{
namespace Blast
{
	/**
	Generic version conversion function for Blast data blocks.

	Automatically determines block type (one of NvBlastDataBlock::Type) and uses appropriate converter.

	\param[out] outBlock		User-supplied memory block to fill with new data.
	\param[in]  inBlock			Data block to convert.
	\param[in]  outBlockVersion	Version to convert too, pass 'nullptr' to convert to the latest version.

	\return	true iff conversion was successful.
	*/
	NVBLAST_API bool convertDataBlock(std::vector<char>& outBlock, const std::vector<char>& inBlock, uint32_t* outBlockVersion = nullptr);


} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTDATACONVERTER_H
