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


#ifndef NVBLASTFIXEDPRIORITYQUEUE_H
#define NVBLASTFIXEDPRIORITYQUEUE_H

#include "NvBlastAssert.h"
#include "NvBlastMemory.h"

namespace Nv
{

namespace Blast
{

/*!
FixedPriorityQueue is a priority queue container which is intended to be used with placement new on chunk of memory.
It'll use following memory for data layout. As follows:

// some memory
char ​*buf = new char[64 *​ 1024];

// placement new on this memory
FixedPriorityQueue<SomeClass>* arr = new (buf) FixedPriorityQueue<SomeClass>();

// you can get max requiredMemorySize by an array of 'capacity' elements count to use memory left
buf = buf + FixedPriorityQueue<SomeClass>::requiredMemorySize(capacity);

buf:

+------------------------------------------------------------+
|  uint32_t  |  T[0]  |  T[1]  |  T[2]  |         ...        |
+------------------------------------------------------------+

*/

template <typename A>
struct Less
{
	bool operator()(const A& a, const A& b) const
	{
		return a < b;
	}
};


template<class Element, class Comparator = Less<Element> >
class FixedPriorityQueue : protected Comparator // inherit so that stateless comparators take no space
{
public:
	FixedPriorityQueue(const Comparator& less = Comparator()) : Comparator(less), mHeapSize(0)
	{
	}
		
	~FixedPriorityQueue()
	{
	}

	static size_t requiredMemorySize(uint32_t capacity)
	{
		return align16(sizeof(FixedPriorityQueue<Element, Comparator>)) + align16(capacity * sizeof(Element));
	}
		
	//! Get the element with the highest priority
	const Element top() const
	{
		return data()[0];
	}

	//! Get the element with the highest priority
	Element top()
	{
		return data()[0];
	}
		
	//! Check to whether the priority queue is empty
	bool empty() const
	{
		return (mHeapSize == 0);
	}
		
	//! Empty the priority queue
	void clear()
	{
		mHeapSize = 0;
	}  

	//! Insert a new element into the priority queue. Only valid when size() is less than Capacity
	void push(const Element& value)
	{
		uint32_t newIndex;
		uint32_t parentIndex = parent(mHeapSize);

		for (newIndex = mHeapSize; newIndex > 0 && compare(value, data()[parentIndex]); newIndex = parentIndex, parentIndex= parent(newIndex)) 
		{
			data()[ newIndex ] = data()[parentIndex];
		}
		data()[newIndex] = value; 
		mHeapSize++;
		NVBLAST_ASSERT(valid());
	}

	//! Delete the highest priority element. Only valid when non-empty.
	Element pop()
	{
		NVBLAST_ASSERT(mHeapSize > 0);
		uint32_t i, child;
		//try to avoid LHS
		uint32_t tempHs = mHeapSize-1;
		mHeapSize = tempHs;
		Element min = data()[0];
		Element last = data()[tempHs];
			
		for (i = 0; (child = left(i)) < tempHs; i = child) 
		{
			/* Find highest priority child */
			const uint32_t rightChild = child + 1;
			
			child += ((rightChild < tempHs) & compare((data()[rightChild]), (data()[child]))) ? 1 : 0;

			if(compare(last, data()[child]))
				break;

			data()[i] = data()[child];
		}
		data()[ i ] = last;
			
		NVBLAST_ASSERT(valid());
		return min;
	} 

	//! Make sure the priority queue sort all elements correctly
	bool valid() const
	{
		const Element& min = data()[0];
		for(uint32_t i=1; i<mHeapSize; ++i)
		{
			if(compare(data()[i], min))
				return false;
		}

		return true;
	}

	//! Return number of elements in the priority queue
	uint32_t size() const
	{
		return mHeapSize;
	}

private:
	uint32_t mHeapSize;

	NV_FORCE_INLINE Element* data()
	{
		return (Element*)((char*)this + sizeof(FixedPriorityQueue<Element, Comparator>));
	}

	NV_FORCE_INLINE Element* data() const
	{
		return (Element*)((char*)this + sizeof(FixedPriorityQueue<Element, Comparator>));
	}

	bool compare(const Element& a, const Element& b) const
	{
		return Comparator::operator()(a,b);
	}

	static uint32_t left(uint32_t nodeIndex) 
	{
		return (nodeIndex << 1) + 1;
	}
		
	static uint32_t parent(uint32_t nodeIndex) 
	{
		return (nodeIndex - 1) >> 1;
	}

	FixedPriorityQueue<Element, Comparator>& operator = (const FixedPriorityQueue<Element, Comparator>);
};

} // namespace Blast
} // namespace Nv

#endif // ifndef NVBLASTFIXEDPRIORITYQUEUE_H
