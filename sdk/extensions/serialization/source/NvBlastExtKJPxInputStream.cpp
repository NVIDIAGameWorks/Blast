/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NvBlastExtKJPxInputStream.h"

namespace Nv
{
	namespace Blast
	{
		ExtKJPxInputStream::ExtKJPxInputStream(capnp::Data::Reader inReader) :
			dataReader(inReader),
			inputStream(nullptr)
		{
			kj::ArrayPtr<const unsigned char> buffer(inReader.begin(), inReader.size());

			inputStream = std::make_shared<kj::ArrayInputStream>(buffer);
		}

		uint32_t ExtKJPxInputStream::read(void* dest, uint32_t count)
		{
			return inputStream->tryRead(dest, count, count);
		}
	}
}

