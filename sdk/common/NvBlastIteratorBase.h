/*
 * Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef NVBLASTITERATORBASE_H
#define NVBLASTITERATORBASE_H


#include "NvBlastIndexFns.h"

namespace Nv
{
namespace Blast
{

/**
Common functionality and implementation for iterators over an index, using invalidIndex<T>() to indicate termination.
Derived class needs to implement increment operators.
*/
template<typename T>
class IteratorBase
{
public:
	/** Constructor sets m_curr value */
	IteratorBase(T curr);

	/** Validity of current value. */
	operator	bool() const;

	/** Current value. */
	operator	T() const;

protected:
	T	m_curr;
};


//////// IteratorBase<T> inline methods ////////

template<typename T>
NV_INLINE IteratorBase<T>::IteratorBase(T curr) : m_curr(curr)
{
}


template<typename T>
NV_INLINE IteratorBase<T>::operator bool() const
{
	return !isInvalidIndex<T>(m_curr);
}


template<typename T>
NV_INLINE IteratorBase<T>::operator T() const
{
	return m_curr;
}


/**
Common functionality and implementation for an indexed linked list iterator
*/
template<typename IndexType>
class LListIt : public IteratorBase<IndexType>
{
public:
	LListIt(IndexType curr, IndexType* links);

	/** Pre-increment.  Only use if valid() == true. */
	uint32_t	operator ++ ();

protected:
	IndexType*	m_links;
};


//////// LListIt<IndexType> inline methods ////////

template<typename IndexType>
NV_INLINE LListIt<IndexType>::LListIt(IndexType curr, IndexType* links) : IteratorBase<IndexType>(curr), m_links(links)
{
}


template<typename IndexType>
NV_INLINE uint32_t LListIt<IndexType>::operator ++ ()
{
	NVBLAST_ASSERT((bool)(*this));
	return (this->m_curr = m_links[this->m_curr]);
}


/**
Common functionality and implementation for an IndexDList<IndexType> iterator
*/
template<typename IndexType>
class DListIt : public IteratorBase<IndexType>
{
public:
	DListIt(IndexType curr, IndexDLink<IndexType>* links);

	/** Pre-increment.  Only use if valid() == true. */
	uint32_t	operator ++ ();

protected:
	IndexDLink<IndexType>*	m_links;
};


//////// DListIt<IndexType> inline methods ////////

template<typename IndexType>
NV_INLINE DListIt<IndexType>::DListIt(IndexType curr, IndexDLink<IndexType>* links) : IteratorBase<IndexType>(curr), m_links(links)
{
}


template<typename IndexType>
NV_INLINE uint32_t DListIt<IndexType>::operator ++ ()
{
	NVBLAST_ASSERT((bool)(*this));
	return (this->m_curr = m_links[this->m_curr].m_adj[1]);
}

} // end namespace Blast
} // end namespace Nv


#endif // #ifndef NVBLASTITERATORBASE_H
