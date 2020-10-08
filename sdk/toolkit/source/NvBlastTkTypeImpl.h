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
// Copyright (c) 2016-2020 NVIDIA Corporation. All rights reserved.


#ifndef NVBLASTTKTYPEIMPL_H
#define NVBLASTTKTYPEIMPL_H


#include "NvPreprocessor.h"

#include "NvBlastTkType.h"


namespace Nv
{
namespace Blast
{

/**
Implementation of TkType, storing class information for TkIdentifiable-derived classes.
*/
class TkTypeImpl : public TkType
{
public:
	TkTypeImpl(const char* typeName, uint32_t typeID, uint32_t version);

	// Begin TkType
	virtual const char*	getName() const override { return getNameInternal(); }

	virtual uint32_t	getVersion() const override { return getVersionInternal(); }
	// End TkType

	// Public methods

	/**
	Access to the class name.

	\return a C string pointer to the class name.
	*/
	const char*			getNameInternal() const;

	/**
	Access to the data format version for the class.

	\return the data format version.
	*/
	uint32_t			getVersionInternal() const;

	/**
	Access to a unique identifier for the class (set using the NVBLASTTK_IMPL_DEFINE_IDENTIFIABLE macro).

	\return the class's unique identifier.
	*/
	uint32_t			getID() const;

	/**
	Access to a runtime-unique small index for the class.

	\return the index for the class.
	*/
	uint32_t			getIndex() const;

	/**
	\return whether or not the index has been set (see setIndex) to a valid value.
	*/
	bool				indexIsValid() const;

private:
	enum { InvalidIndex = 0xFFFFFFFF };

	/**
	Sets the type index.

	\param[in]	index	The index to set.
	*/
	void				setIndex(uint32_t index);

	const char*		m_name;				//!<	The name of the class, set by the constructor.
	uint32_t		m_ID;				//!<	The unique identifier for the class, set by the constructor. 
	uint32_t		m_version;			//!<	The data format version for the class, set by the constructor.
	uint32_t		m_index;			//!<	The index set for this class, set using setIndex().

	friend class TkFrameworkImpl;
};


//////// TkTypeImpl inline methods ////////

NV_INLINE TkTypeImpl::TkTypeImpl(const char* typeName, uint32_t typeID, uint32_t version)
	: m_name(typeName)
	, m_ID(typeID)
	, m_version(version)
	, m_index((uint32_t)InvalidIndex)
{
}


NV_INLINE const char* TkTypeImpl::getNameInternal() const
{
	return m_name;
}


NV_INLINE uint32_t TkTypeImpl::getVersionInternal() const
{
	return m_version;
}


NV_INLINE uint32_t TkTypeImpl::getID() const
{
	return m_ID;
}


NV_INLINE uint32_t TkTypeImpl::getIndex() const
{
	return m_index;
}


NV_INLINE bool TkTypeImpl::indexIsValid() const
{
	return m_index != (uint32_t)InvalidIndex;
}


NV_INLINE void TkTypeImpl::setIndex(uint32_t index)
{
	m_index = index;
}

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTTKTYPEIMPL_H
