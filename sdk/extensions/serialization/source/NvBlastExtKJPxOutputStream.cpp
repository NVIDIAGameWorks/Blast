/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NvBlastExtKJPxOutputStream.h"

namespace Nv
{
	namespace Blast
	{
		ExtKJPxOutputStream::ExtKJPxOutputStream(kj::ArrayPtr<unsigned char> inBuffer) :
			writtenBytes(0),
			Buffer(inBuffer),
			outputStream(nullptr)
		{
			outputStream = std::make_shared<kj::ArrayOutputStream>(inBuffer);
		}

		uint32_t ExtKJPxOutputStream::write(const void* src, uint32_t count)
		{
			outputStream->write(src, count);

			writtenBytes += count;

			return count;
		}
	}
}

