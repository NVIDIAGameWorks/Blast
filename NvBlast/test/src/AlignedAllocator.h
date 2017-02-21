/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef ALIGNEDALLOCATOR_H
#define ALIGNEDALLOCATOR_H

#include "NvPreprocessor.h"


/**
Aligned allocation.  First template argument has the signature of stdlib malloc.

Example using malloc and 16-byte alignment:

// b will lie on a 16-byte boundary and point to 50 bytes of usable memory
void* b = alignedAlloc<malloc,16>(50);
*/
template<void*(*allocFn)(size_t), int A>
void* alignedAlloc(size_t size)
{
	NV_COMPILE_TIME_ASSERT(A > 0 && A <= 256);
	unsigned char* mem = (unsigned char*)allocFn(size + A);
	const unsigned char offset = (unsigned char)((uintptr_t)A - (uintptr_t)mem % A - 1);
	mem += offset;
	*mem++ = offset;
	return mem;
};


/**
Version of alignedAlloc specialized 16-byte aligned allocation.
*/
template<void*(*allocFn)(size_t)>
void* alignedAlloc(size_t size)
{
	return alignedAlloc<allocFn, 16>(size);
}


/**
Aligned deallocation.  First template argument has the signature of stdlib free.

Memory freed using this function MUST have been allocated using alignedAlloc.

Example using free:

// Using the memory pointer b from the example above (for alignedAlloc)
alignedFree<free>(b);
*/
template<void(*freeFn)(void*)>
void alignedFree(void* block)
{
	if (block != nullptr)
	{
		unsigned char* mem = (unsigned char*)block;
		const unsigned char offset = *--mem;
		freeFn(mem - offset);
	}
};


#endif // ALIGNEDALLOCATOR_H
