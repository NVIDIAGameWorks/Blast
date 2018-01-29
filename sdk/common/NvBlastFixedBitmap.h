// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2016-2018 NVIDIA Corporation. All rights reserved.


#ifndef NVBLASTFIXEDBITMAP_H
#define NVBLASTFIXEDBITMAP_H

#include "NvBlastAssert.h"
#include "NvBlastMemory.h"
#include <cstring>

namespace Nv
{
namespace Blast
{

/*!
FixedBitmap is a bitset (bitmap) of fixed side, it's intended to be used with placement new on chunk of memory.
It'll use following memory for data layout. As follows:

// some memory
char ​*buf = new char[64 *​ 1024];

const uint32_t bitsCount = 100;

// placement new on this memory
FixedBitmap* arr = new (buf) FixedBitmap(bitsCount);

// you can get max requiredMemorySize by an bitMap to use memory left
buf = buf + FixedBitmap<SomeClass>::requiredMemorySize(bitsCount);

buf:

+------------------------------------------------------------+
|  uint32_t  |  word0  |  word1  |  word2  |       ...       |
+------------------------------------------------------------+

*/
class FixedBitmap
{
public:
	explicit FixedBitmap(uint32_t bitsCount)
	{
		m_bitsCount = bitsCount;
	}

	static uint32_t getWordsCount(uint32_t bitsCount)
	{
		return (bitsCount + 31) >> 5;
	}

	static size_t requiredMemorySize(uint32_t bitsCount)
	{
		return align16(sizeof(FixedBitmap)) + align16(getWordsCount(bitsCount) * sizeof(uint32_t));
	}

	void clear()
	{
		memset(data(), 0, getWordsCount(m_bitsCount) * sizeof(uint32_t));
	}

	void fill()
	{
		const uint32_t wordCount = getWordsCount(m_bitsCount);
		uint32_t* mem = data();
		memset(mem, 0xFF, wordCount * sizeof(uint32_t));
		const uint32_t bitsRemainder = m_bitsCount & 31;
		if (bitsRemainder > 0)
		{
			mem[wordCount - 1] &= ~(0xFFFFFFFF << bitsRemainder);
		}
	}

	int test(uint32_t index) const
	{
		NVBLAST_ASSERT(index < m_bitsCount);
		return data()[index >> 5] & (1 << (index & 31));
	}

	void set(uint32_t index)
	{
		NVBLAST_ASSERT(index < m_bitsCount);
		data()[index >> 5] |= 1 << (index & 31);
	}

	void reset(uint32_t index)
	{
		NVBLAST_ASSERT(index < m_bitsCount);
		data()[index >> 5] &= ~(1 << (index & 31));
	}

private:
	uint32_t m_bitsCount;

	NV_FORCE_INLINE uint32_t* data()
	{
		return (uint32_t*)((char*)this + sizeof(FixedBitmap));
	}

	NV_FORCE_INLINE const uint32_t* data() const
	{
		return (uint32_t*)((char*)this + sizeof(FixedBitmap));
	}

private:
	FixedBitmap(const FixedBitmap& that);
};

} // namespace Blast
} // namespace Nv

#endif // ifndef NVBLASTFIXEDBITMAP_H
