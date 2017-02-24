/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NvBlastExtInputStream.h"


Nv::Blast::ExtInputStream::ExtInputStream(std::istream &inputStream):
	m_inputStream(inputStream)
{

}


size_t Nv::Blast::ExtInputStream::tryRead(void* buffer, size_t /*minBytes*/, size_t maxBytes)
{
	m_inputStream.read((char *) buffer, maxBytes);

	if (m_inputStream.fail())
	{
		// Throw exception, log error
//		NVBLASTEXT_LOG_ERROR("Failure when reading from stream");
	}

	// Since we're using a blocking read above, if we don't have maxBytes we're probably done
	if ((size_t) m_inputStream.gcount() < maxBytes)
	{
//		NVBLASTEXT_LOG_ERROR("Failed to read requested number of bytes during blocking read.");
	}

	return m_inputStream.gcount();
}
