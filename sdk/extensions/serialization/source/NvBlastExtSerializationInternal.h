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
// Copyright (c) 2020 NVIDIA Corporation. All rights reserved.


#pragma once

#include "NvBlastExtSerialization.h"

#include <cstring>

#define ExtSerializerBoilerplate(_name, _description, _objectTypeID, _encodingID)	\
virtual const char*	getName() const override { return _name; }						\
virtual const char*	getDescription() const override { return _description; }		\
virtual uint32_t	getObjectTypeID() const override { return _objectTypeID; }		\
virtual uint32_t	getEncodingID() const override { return _encodingID; }


#define ExtSerializerReadOnly(_name)																\
virtual bool		isReadOnly() const override { return true; }									\
virtual uint64_t	serializeIntoBuffer																\
(																									\
	void*& buffer,																					\
	ExtSerialization::BufferProvider& bufferProvider,												\
	const void* object,																				\
	uint64_t offset = 0																				\
) override																							\
{																									\
	NVBLAST_LOG_WARNING(#_name "::serializeIntoBuffer: ExtPxAsset_RAW serializer is read-only.");	\
	NV_UNUSED(buffer);																				\
	NV_UNUSED(bufferProvider);																		\
	NV_UNUSED(object);																				\
	NV_UNUSED(offset);																				\
	return 0;																						\
}


#define ExtSerializerDefaultFactoryAndRelease(_classname)	\
static ExtSerializer*	create()							\
{															\
	return NVBLAST_NEW(_classname) ();						\
}															\
virtual void release() override								\
{															\
	NVBLAST_DELETE(this, _classname);						\
}


namespace Nv
{
namespace Blast
{

/**
Serializer internal interface
*/
class ExtSerializer
{
public:
	virtual				~ExtSerializer() {}

	/**
	return the name of this serializer.
	*/
	virtual const char*	getName() const = 0;

	/**
	return a description of this serializer.
	*/
	virtual const char*	getDescription() const = 0;

	/**
	return an identifier for the type of object handled.
	*/
	virtual uint32_t	getObjectTypeID() const = 0;

	/**
	return an identifier for serialization format.
	*/
	virtual uint32_t	getEncodingID() const = 0;

	/**
	Whether or not this serializer supports writing.  Legacy formats, for example, may not.

	\return true iff this serialization does not support writing.
	*/
	virtual bool		isReadOnly() const { return false; }

	/**
	Deserialize from a buffer into a newly allocated object.

	\param[in] buffer	Pointer to the buffer to read.
	\param[in] size		Size of the buffer to read.
	\return object pointer; returns null if failed to deserialize.
	*/
	virtual void*		deserializeFromBuffer(const void* buffer, uint64_t size) = 0;

	/**
	Serialize into a buffer.  Allocates the buffer internally using the ExtSerialization::BufferProvider callack interface.

	\param[out]	buffer			Pointer to the buffer created.
	\param[in]	bufferProvider	The buffer provider callback interface to use.
	\param[in]	object			Object pointer.

	\return the number of bytes serialized into the buffer (zero if unsuccessful).
	*/
	virtual uint64_t	serializeIntoBuffer(void*& buffer, ExtSerialization::BufferProvider& bufferProvider, const void* object, uint64_t offset = 0) = 0;

	/**
	Release the serializer and free associated memory.
	*/
	virtual void		release() = 0;
};


/**
Internal serialization manager interface
*/
class ExtSerializationInternal : public ExtSerialization
{
public:
	/**
	Internal interfaces to register and unregister a serializer, used by modules to automatically
	register all of their serializers with a serialization manager.
	*/
	virtual bool			registerSerializer(ExtSerializer& serializer) = 0;
	virtual bool			unregisterSerializer(ExtSerializer& serializer) = 0;

	/**
	Find a registered serializer for the given object type and encoding.

	\param[in]	objectTypeID	ID for the requested object type.
	\param[in]	encodingID		ID for the requested encoding (see EncodingID).

	\return a registered serializer if found, NULL otherwise.
	*/
	virtual ExtSerializer*	findSerializer(uint32_t objectTypeID, uint32_t encodingID) = 0;

	//// Enums ////
	enum { HeaderSize = 128 };
};


template<typename Factory, size_t N>
size_t ExtSerializationLoadSet(Nv::Blast::ExtSerializationInternal& serialization, Factory(&factories)[N])
{
	size_t count = 0;

	for (auto f : factories)
	{
		Nv::Blast::ExtSerializer* serializer = f();
		if (serializer != nullptr)
		{
			if (serialization.registerSerializer(*serializer))
			{
				++count;
			}
			else
			{
				NVBLAST_LOG_ERROR("Nv::Blast::ExtSerializationLoadSet: failed to register serailizer:");
				NVBLAST_LOG_ERROR(serializer->getName());
				serializer->release();
			}
		}
		else
		{
			NVBLAST_LOG_ERROR("Nv::Blast::ExtSerializationLoadSet: failed to create serailizer.");
		}
	}

	return count;
}


class ExtIStream
{
public:
	enum Flags
	{
		LittleEndian = (1 << 0),
		Fail = (1 << 1)
	};

	ExtIStream(const void* buffer, size_t size) : m_buf(reinterpret_cast<const char*>(buffer)), m_flags(0)
	{
		m_cur = m_buf;
		m_end = m_buf + size;
		const uint16_t x = LittleEndian;
		m_flags = *reinterpret_cast<const char*>(&x);
	}

	bool	advance(ptrdiff_t diff)
	{
		m_cur += diff;
		if (m_cur < m_buf)
		{
			m_cur = m_buf;
			m_flags |= Fail;
			return false;
		}
		else
		if (m_cur > m_end)
		{
			m_cur = m_end;
			m_flags |= Fail;
			return false;
		}
		return true;
	}

	const void*	view()
	{
		return m_cur;
	}

	bool	read(void* buffer, size_t size)
	{
		if (!canRead(size)) return false;
		std::memcpy(buffer, m_cur, size);
		m_cur += size;
		return true;
	}

	size_t	tellg() const { return m_cur - m_buf; }
	size_t	left()	const { return m_end - m_cur; }

	bool	eof() const { return m_cur >= m_end; }
	bool	fail() const { return (m_flags & Fail) != 0; }

private:
	const char*	m_buf;
	const char*	m_cur;
	const char*	m_end;
	uint32_t	m_flags;

	bool	isLittleEndian() const { return (m_flags & LittleEndian) != 0; }

	bool	canRead(size_t size) const { return m_cur + size <= m_end; }

	template<typename T>
	friend	ExtIStream& operator >> (ExtIStream& s, T& x);
};

template<typename T>
NV_INLINE ExtIStream& operator >> (ExtIStream& s, T& x)
{
	if (s.canRead(sizeof(T)))
	{
		if (s.isLittleEndian())
		{
			x = *reinterpret_cast<const T*>(s.m_cur);
			s.m_cur += sizeof(T);
		}
		else
		{
			char* b = reinterpret_cast<char*>(&x) + sizeof(T);
			for (size_t n = sizeof(T); n--;) *--b = *s.m_cur++;
		}
	}
	else
	{
		s.m_flags |= ExtIStream::Fail;
	}
	return s;
}

}	// namespace Blast
}	// namespace Nv
