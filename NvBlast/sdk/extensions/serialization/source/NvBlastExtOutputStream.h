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
#include "kj/io.h"
#include <ostream>

namespace Nv
{
	namespace Blast
	{
		class ExtOutputStream : public kj::OutputStream
		{

		public:
			ExtOutputStream() = delete;
			ExtOutputStream(std::ostream &outputStream);

			virtual void write(const void* buffer, size_t size) override;
		private:
			std::ostream &m_outputStream;
		};
	}
}
