/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once
#include "PxIO.h"
#include "kj/common.h"
#include <memory>
#include "kj/io.h"

namespace Nv
{
	namespace Blast
	{
		class ExtKJPxOutputStream : public physx::PxOutputStream
		{
		public:
			ExtKJPxOutputStream(kj::ArrayPtr<unsigned char> inBuffer);
			~ExtKJPxOutputStream() = default;

			virtual uint32_t write(const void* src, uint32_t count) override;

			uint32_t getWrittenBytes() { return writtenBytes; }

			kj::ArrayPtr<unsigned char> getBuffer() { return Buffer; }

		private:
			uint32_t writtenBytes;

			kj::ArrayPtr<unsigned char> Buffer;
			std::shared_ptr<kj::ArrayOutputStream> outputStream;
		};
	}
}
