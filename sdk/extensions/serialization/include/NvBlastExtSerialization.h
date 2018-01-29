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
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.


#pragma once

#include "NvBlastGlobals.h"


namespace Nv
{
namespace Blast
{

/**
	Serialization manager interface
*/
class ExtSerialization
{
public:
	/** Standard Encoding IDs */
	struct EncodingID
	{
		enum Enum
		{
			CapnProtoBinary =	NVBLAST_FOURCC('C', 'P', 'N', 'B'),
			RawBinary =			NVBLAST_FOURCC('R', 'A', 'W', ' '),
		};
	};

	/** Buffer provider API, used to request a buffer for serialization. */
	class BufferProvider
	{
	public:
		virtual void*	requestBuffer(size_t size) = 0;
	};

	/**
	Set the serialization encoding to use.  (See EncodingID.)

	\return true iff successful.
	*/
	virtual bool		setSerializationEncoding(uint32_t encodingID) = 0;

	/**
	Retrieve the current serialization encoding being used.  Note, by default this is set to the encoding of the first
	serializer registered by a module.  Currently this is done automatically by the NvBlastExtLlExtension module.

	\return the current serialization encoding (zero if none is set).
	*/
	virtual uint32_t	getSerializationEncoding() const = 0;

	/**
	Set the buffer provider callback to use.  (See BufferProvider.)  If not set, a default provider using NVBLAST_ALLOC (see
	NvBlastGlobals.h) is used, which may be freed using NvBLAST_FREE.

	\param[in]	bufferProvider	Buffer provider callback to use.  If NULL, uses the default provider using the allocator given
	in NvBlastGlobals.	
	*/
	virtual void		setBufferProvider(BufferProvider* bufferProvider) = 0;

	/**
	Reads information from a buffer, returning the contained object type ID, encoding ID, and data size.
	\param[out]	objectTypeID	If not NULL, the object type ID is written to *objectTypeID.
	\param[out]	encodingID		If not NULL, the encoding ID is written to *encodingID.
	\param[out]	dataSize		If not NULL, the data size is written to *dataSize.  (Does not include the size of the header.)
	\param[in]	buffer			Pointer to the buffer to read.
	\param[in]	bufferSize		Size of the buffer to read.

	\return true iff the header is successfully read.
	*/
	virtual bool		peekHeader(uint32_t* objectTypeID, uint32_t* encodingID, uint64_t* dataSize, const void* buffer, uint64_t bufferSize) = 0;

	/**
	Determines the current object in the buffer and returns the position in the buffer immediately after the object.

	\param[in, out] bufferSize	Size of the buffer to read on input, on output the remaining buffer size given the return buffer value.
	\param[in] buffer			Pointer to the buffer to read.

	\return a pointer to the new position in the buffer after the skipped object if successful, NULL otherwise.  The bufferSize field is only updated if a valid pointer is returned.
	*/
	virtual const void*	skipObject(uint64_t& bufferSize, const void* buffer) = 0;

	/**
	Deserialize from a buffer into a newly allocated object.

	\param[in]	buffer			Pointer to the buffer to read.
	\param[in]	bufferSize		Size of the buffer to read.
	\param[out]	objectTypeIDPtr	Optional, if not NULL then *objectTypeIDPtr will be filled with the deserialized object's
								type ID if deserialization is successful, or 0 if unsuccessful.

	\return object pointer; returns null if failed to deserialize.
	*/
	virtual void*		deserializeFromBuffer(const void* buffer, uint64_t bufferSize, uint32_t* objectTypeIDPtr = nullptr) = 0;

	/**
	Serialize into a buffer.  Allocates the buffer internally using the callack set in setBufferProvider.

	\param[out]	buffer			Pointer to the buffer created.
	\param[in]	object			Object pointer.
	\param[in]	objectTypeID	Object type ID.

	\return the number of bytes serialized into the buffer (zero if unsuccessful).
	*/
	virtual uint64_t	serializeIntoBuffer(void*& buffer, const void* object, uint32_t objectTypeID) = 0;

	/**
	Release the serialization manager and all contained objects.
	*/
	virtual void		release() = 0;

protected:
	/**
	Destructor is virtual and not public - use the release() method instead of explicitly deleting the serialization manager
	*/
	virtual				~ExtSerialization() {}
};

}	// namespace Blast
}	// namespace Nv


//////// Global API to create serialization ////////

/**
Create a new serialization manager.  To release it, use its release() method.

This uses the global allocator set in NvBlastGlobals.h.

\return a new serialization manager.
*/
NVBLAST_API Nv::Blast::ExtSerialization*	NvBlastExtSerializationCreate();
