/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

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
