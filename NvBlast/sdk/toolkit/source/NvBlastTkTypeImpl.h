/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTTKTYPEIMPL_H
#define NVBLASTTKTYPEIMPL_H


#include "NvPreprocessor.h"

#include "NvBlastTkType.h"


// Forward declarations
namespace physx
{
namespace general_PxIOStream2
{
class PxFileBuf;
}
}


namespace Nv
{
namespace Blast
{

// Forward declarations
class TkSerializable;


// Serialization function signature
typedef TkSerializable* (*TkDeserializeFn)(physx::general_PxIOStream2::PxFileBuf&, const NvBlastID& id);


/**
Implementation of TkType, storing class information for TkIdentifiable-derived classes.
*/
class TkTypeImpl : public TkType
{
public:
	TkTypeImpl(const char* typeName, uint32_t typeID, uint32_t version, TkDeserializeFn deserializeFn);

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
	Access to the data format version for the class (used if it TkSerializable-derived).

	\return the data format version.
	*/
	uint32_t			getVersionInternal() const;

	/**
	Access to a unique identifier for the class (set using the NVBLASTTK_IMPL_DEFINE_IDENTIFIABLE or NVBLASTTK_IMPL_DEFINE_SERIALIZABLE macro).

	\return the class's unique identifier.
	*/
	uint32_t			getID() const;

	/**
	\return the class's deserialization function.
	*/
	TkDeserializeFn		getDeserializeFn() const;

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
	TkDeserializeFn	m_deserializeFn;	//!<	The class deserialization function, set by the constructor.
	uint32_t		m_index;			//!<	The index set for this class, set using setIndex().

	friend class TkFrameworkImpl;
};


//////// TkTypeImpl inline methods ////////

NV_INLINE TkTypeImpl::TkTypeImpl(const char* typeName, uint32_t typeID, uint32_t version, TkDeserializeFn deserializeFn)
	: m_name(typeName)
	, m_ID(typeID)
	, m_version(version)
	, m_deserializeFn(deserializeFn)
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


NV_INLINE TkDeserializeFn TkTypeImpl::getDeserializeFn() const
{
	return m_deserializeFn;
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
