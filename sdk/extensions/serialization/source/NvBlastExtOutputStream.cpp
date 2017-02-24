/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NvBlastExtOutputStream.h"



Nv::Blast::ExtOutputStream::ExtOutputStream(std::ostream &outputStream):
	m_outputStream(outputStream)
{

}

void Nv::Blast::ExtOutputStream::write(const void* buffer, size_t size)
{
	m_outputStream.write((char *) buffer, size);
}
