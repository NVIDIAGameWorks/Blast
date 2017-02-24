/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTTKSERIALIZABLE_H
#define NVBLASTTKSERIALIZABLE_H


#include "NvBlastTkIdentifiable.h"


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

/**
TkSerializable objects support the serialize interface, and are returned by TkFramework::deserialize.
*/
class TkSerializable : public TkIdentifiable
{
public:
	/**
	Write the object data to the user-defined PxFileBuf stream.

	\param[in]	stream	User-defined stream object.

	\return true if serialization was successful, false otherwise.
	*/
	virtual bool	serialize(physx::general_PxIOStream2::PxFileBuf& stream) const = 0;

	// Data

	/**
	Integer field available to the user.  This data is serialized.
	*/
	uint64_t	userIntData;
};

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTTKSERIALIZABLE_H
