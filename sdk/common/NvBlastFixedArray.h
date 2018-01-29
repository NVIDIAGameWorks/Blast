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


#ifndef NVBLASTFIXEDARRAY_H
#define NVBLASTFIXEDARRAY_H

#include "NvBlastAssert.h"
#include "NvBlastMemory.h"

namespace Nv
{
namespace Blast
{

/*!
FixedArray is a sequential container which is intended to be used with placement new on chunk of memory.
It'll use following memory for data layout. As follows:

// some memory
char ​*buf = new char[64 *​ 1024];

// placement new on this memory
FixedArray<SomeClass>* arr = new (buf) FixedArray<SomeClass>();

// you can get max requiredMemorySize by an array of 'capacity' elements count to use memory left
buf = buf + FixedArray<SomeClass>::requiredMemorySize(capacity);

buf:

+------------------------------------------------------------+
|  uint32_t  |  T[0]  |  T[1]  |  T[2]  |         ...        |
+------------------------------------------------------------+


!!!TODO:
- check ctor/dtor of elements calls
*/
template <class T>
class FixedArray
{
public:
	explicit FixedArray() : m_size(0)
	{
	}

	static size_t requiredMemorySize(uint32_t capacity)
	{
		return align16(sizeof(FixedArray<T>)) + align16(capacity * sizeof(T));
	}

	NV_FORCE_INLINE T& pushBack(T& t)
	{
		new (data() + m_size) T(t);
		return data()[m_size++];
	}

	T popBack()
	{
		NVBLAST_ASSERT(m_size);
		T t = data()[m_size - 1];
		data()[--m_size].~T();
		return t;
	}

	void clear()
	{
		for(T* first = data(); first < data() + m_size; ++first)
			first->~T();
		m_size = 0;
	}

	NV_FORCE_INLINE void forceSize_Unsafe(uint32_t s)
	{
		m_size = s;
	}

	NV_FORCE_INLINE T& operator[](uint32_t idx)
	{
		NVBLAST_ASSERT(idx < m_size);
		return data()[idx];
	}

	NV_FORCE_INLINE const T& operator[](uint32_t idx) const
	{
		NVBLAST_ASSERT(idx < m_size);
		return data()[idx];
	}

	NV_FORCE_INLINE T& at(uint32_t idx)
	{
		NVBLAST_ASSERT(idx < m_size);
		return data()[idx];
	}

	NV_FORCE_INLINE const T& at(uint32_t idx) const
	{
		NVBLAST_ASSERT(idx < m_size);
		return data()[idx];
	}

	NV_FORCE_INLINE uint32_t size() const
	{
		return m_size;
	}

private:
	uint32_t m_size;

	NV_FORCE_INLINE T* data()
	{
		return (T*)((char*)this + sizeof(FixedArray<T>));
	}

private:
	FixedArray(const FixedArray& that);
};

} // namespace Blast
} // namespace Nv

#endif // ifndef NVBLASTFIXEDARRAY_H
