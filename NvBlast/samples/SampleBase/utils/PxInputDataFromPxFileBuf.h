/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef PXINPUTDATAFROMPXFILEBUF_H
#define PXINPUTDATAFROMPXFILEBUF_H

#include <PsFileBuffer.h>


// Copied from APEX 
class PxInputDataFromPxFileBuf : public physx::PxInputData
{
public:
	PxInputDataFromPxFileBuf(physx::PxFileBuf& fileBuf) : mFileBuf(fileBuf) {}

	// physx::PxInputData interface
	virtual uint32_t	getLength() const
	{
		return mFileBuf.getFileLength();
	}

	virtual void	seek(uint32_t offset)
	{
		mFileBuf.seekRead(offset);
	}

	virtual uint32_t	tell() const
	{
		return mFileBuf.tellRead();
	}

	// physx::PxInputStream interface
	virtual uint32_t read(void* dest, uint32_t count)
	{
		return mFileBuf.read(dest, count);
	}

	PX_NOCOPY(PxInputDataFromPxFileBuf)
private:
	physx::PxFileBuf& mFileBuf;
};


#endif //PXINPUTDATAFROMPXFILEBUF_H