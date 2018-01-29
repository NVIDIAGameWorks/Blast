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
