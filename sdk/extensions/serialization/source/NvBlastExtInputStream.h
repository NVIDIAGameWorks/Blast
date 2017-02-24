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
#include <istream>

namespace Nv
{
	namespace Blast
	{
		class ExtInputStream : public kj::InputStream
		{
			public:
				ExtInputStream() = delete;
				ExtInputStream(std::istream &inputStream);

				// Returns a read of maxBytes. This is supposed to be happy doing partial reads, but currently isn't. 
				virtual size_t tryRead(void* buffer, size_t minBytes, size_t maxBytes) override;

			private:
				std::istream &m_inputStream;
		};
	}
}