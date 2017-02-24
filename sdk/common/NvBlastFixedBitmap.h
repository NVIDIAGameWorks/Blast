/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

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
