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
#include "capnp/common.h"
#include "kj/io.h"
#include <memory>
#include "generated/NvBlastExtSerialization.capn.h"

namespace Nv
{
	namespace Blast
	{
		/*
		A wrapper around a Capn Proto Data reader.

		Since it needs to behave like a stream, it's internally wrapped in a stream.

		*/
		class ExtKJPxInputStream : public physx::PxInputStream
		{
		public:
			ExtKJPxInputStream(capnp::Data::Reader inReader);
			~ExtKJPxInputStream() = default;

			virtual uint32_t read(void* dest, uint32_t count) override;

		private:
			capnp::Data::Reader dataReader;
			std::shared_ptr<kj::ArrayInputStream> inputStream;
		};
	}
}
