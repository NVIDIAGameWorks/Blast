/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTTKALLOCATOR_H
#define NVBLASTTKALLOCATOR_H

#include "PxAllocatorCallback.h"


namespace Nv
{
namespace Blast
{

/**
An allocator which can be used in PxShared containers.
*/
class TkAllocator
{
public:
	TkAllocator(const char* = 0)
	{
	}

	void* allocate(size_t size, const char* file, int line)
	{
		return s_allocatorCallback->allocate(size, nullptr, file, line);
	}

	void deallocate(void* ptr)
	{
		return s_allocatorCallback->deallocate(ptr);
	}

	static physx::PxAllocatorCallback* s_allocatorCallback;
};

} // namespace Blast
} // namespace Nv


#endif // #ifndef NVBLASTTKALLOCATOR_H
