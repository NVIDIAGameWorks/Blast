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


#ifndef NVBLASTFIXEDQUEUE_H
#define NVBLASTFIXEDQUEUE_H

#include "NvBlastAssert.h"
#include "NvBlastMemory.h"

namespace Nv
{
namespace Blast
{

/*!
FixedQueue is a queue container which is intended to be used with placement new on chunk of memory.
It'll use following memory for data layout. As follows:

// some memory
char ​*buf = new char[64 *​ 1024];

// placement new on this memory
FixedQueue<SomeClass>* arr = new (buf) FixedQueue<SomeClass>();

// you can get max requiredMemorySize by an array of 'capacity' elements count to use memory left
buf = buf + FixedQueue<SomeClass>::requiredMemorySize(capacity);

*/
template <class T>
class FixedQueue
{
public:
	explicit FixedQueue(uint32_t maxEntries) : m_num(0), m_head(0), m_tail(0), m_maxEntries(maxEntries)
	{
	}

	static size_t requiredMemorySize(uint32_t capacity)
	{
		return align16(sizeof(FixedQueue<T>)) + align16(capacity * sizeof(T));
	}

	T popFront()
	{
		NVBLAST_ASSERT(m_num>0);

		m_num--;
		T& element = data()[m_tail];
		m_tail = (m_tail+1) % (m_maxEntries);
		return element;
	}

	T front()
	{
		NVBLAST_ASSERT(m_num>0);

		return data()[m_tail];
	}

	T popBack()
	{
		NVBLAST_ASSERT(m_num>0);

		m_num--;
		m_head = (m_head-1) % (m_maxEntries);
		return data()[m_head];
	}

	T back()
	{
		NVBLAST_ASSERT(m_num>0);

		uint32_t headAccess = (m_head-1) % (m_maxEntries);
		return data()[headAccess];
	}

	bool pushBack(const T& element)
	{
		if (m_num == m_maxEntries) return false;
		data()[m_head] = element;

		m_num++;
		m_head = (m_head+1) % (m_maxEntries);

		return true;
	}

	bool empty() const
	{
		return m_num == 0;
	}

	uint32_t size() const
	{
		return m_num;
	}	


private:
	uint32_t		m_num;
	uint32_t		m_head;
	uint32_t		m_tail;
	uint32_t		m_maxEntries;

	T* data()
	{
		return (T*)((char*)this + sizeof(FixedQueue<T>));
	}

private:
	FixedQueue(const FixedQueue& that);
};

} // namespace Blast
} // namespace Nv

#endif // ifndef NVBLASTFIXEDQUEUE_H
