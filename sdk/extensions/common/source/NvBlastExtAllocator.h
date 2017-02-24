/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTEXTALLOCATOR_H
#define NVBLASTEXTALLOCATOR_H

#include "NvBlastTkFramework.h"
#include "PxAllocatorCallback.h"


namespace Nv
{
namespace Blast
{

/**
ExtAllocator uses TkFramework allocator
*/
class ExtAllocator
{
public:
	ExtAllocator(const char* = 0)
	{
	}

	void* allocate(size_t size, const char* filename, int line)
	{
		return NvBlastTkFrameworkGet()->getAllocatorCallback().allocate(size, nullptr, filename, line);
	}

	void deallocate(void* ptr)
	{
		NvBlastTkFrameworkGet()->getAllocatorCallback().deallocate(ptr);
	}


	/**
	Aligned allocation.

	Example using 16-byte alignment:

	// b will lie on a 16-byte boundary and point to 50 bytes of usable memory
	void* b = alignedAlloc<16>(50);
	*/
	template<int A>
	static void* alignedAlloc(size_t size, const char* filename, int line)
	{
		NV_COMPILE_TIME_ASSERT(A > 0 && A <= 256);
		unsigned char* mem = (unsigned char*)ExtAllocator().allocate(size + A, filename, line);
		const unsigned char offset = (unsigned char)((uintptr_t)A - (uintptr_t)mem % A - 1);
		mem += offset;
		*mem++ = offset;
		return mem;
	}

	template<int A>
	static void* alignedAlloc(size_t size)
	{
		return alignedAlloc<A>(size, __FILE__, __LINE__);
	}


	/**
	Version of alignedAlloc specialized 16-byte aligned allocation.
	*/
	static void* alignedAlloc16(size_t size)
	{
		return alignedAlloc<16>(size);
	}


	/**
	Aligned deallocation.

	Memory freed using this function MUST have been allocated using alignedAlloc.

	Example using free:

	// Using the memory pointer b from the example above (for alignedAlloc)
	alignedFree(b);
	*/
	static void alignedFree(void* block)
	{
		if (block != nullptr)
		{
			unsigned char* mem = (unsigned char*)block;
			const unsigned char offset = *--mem;
			ExtAllocator().deallocate(mem - offset);
		}
	};
};


/**
ExtAlignedAllocator uses ExtAllocator
*/
template<int A>
class ExtAlignedAllocator
{
public:
	ExtAlignedAllocator(const char* = 0)
	{
	}

	void* allocate(size_t size, const char* filename, int line)
	{
		return ExtAllocator::alignedAlloc<A>(size, filename, line);
	}

	void deallocate(void* ptr)
	{
		return ExtAllocator::alignedFree(ptr);
	}
};

} // namespace Blast
} // namespace Nv


#endif // #ifndef NVBLASTEXTALLOCATOR_H
