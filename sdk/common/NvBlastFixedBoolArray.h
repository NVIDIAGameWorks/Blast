/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTFIXEDBOOLARRAY_H
#define NVBLASTFIXEDBOOLARRAY_H

#include "NvBlastAssert.h"
#include "NvBlastMemory.h"
#include <cstring>

namespace Nv
{
namespace Blast
{

/*!
FixedBoolArray is an array of bools of fixed size, it's intended to be used with placement new on chunk of memory.
It'll use following memory for data layout. As follows:

// some memory
char ​*buf = new char[64 *​ 1024];

const uint32_t size = 100;

// placement new on this memory
FixedBoolArray* arr = new (buf) FixedBoolArray(size);

// you can get max requiredMemorySize by an bitMap to use memory left
buf = buf + FixedBoolArray<SomeClass>::requiredMemorySize(size);

buf:

+------------------------------------------------------------+
|  uint32_t  |  bool0  |  bool1  |  bool2  |       ...       |
+------------------------------------------------------------+

*/
class FixedBoolArray
{
public:
	explicit FixedBoolArray(uint32_t size)
	{
		m_size = size;
	}

	static size_t requiredMemorySize(uint32_t size)
	{
		return align16(sizeof(FixedBoolArray)) + align16(size);
	}

	void clear()
	{
		memset(data(), 0, m_size);
	}

	void fill()
	{
		memset(data(), 1, m_size);
	}

	int test(uint32_t index) const
	{
		NVBLAST_ASSERT(index < m_size);
		return data()[index];
	}

	void set(uint32_t index)
	{
		NVBLAST_ASSERT(index < m_size);
		data()[index] = 1;
	}

	void reset(uint32_t index)
	{
		NVBLAST_ASSERT(index < m_size);
		data()[index] = 0;
	}

private:
	uint32_t m_size;

	NV_FORCE_INLINE char* data()
	{
		return ((char*)this + sizeof(FixedBoolArray));
	}

	NV_FORCE_INLINE const char* data() const
	{
		return ((char*)this + sizeof(FixedBoolArray));
	}

private:
	FixedBoolArray(const FixedBoolArray& that);
};

} // namespace Blast
} // namespace Nv

#endif // ifndef NVBLASTFIXEDBOOLARRAY_H
