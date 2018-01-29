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


/**
Blast Low-level serialization support.  Contains serializers which can be used by the ExtSerialization manager.
*/


// Forward declarations
struct NvBlastAsset;
struct NvBlastFamily;


namespace Nv
{
namespace Blast
{

// Forward declarations
class ExtSerialization;


/** Standard Object Type IDs */
struct LlObjectTypeID
{
	enum Enum
	{
		Asset =		NVBLAST_FOURCC('L', 'L', 'A', 'S'),
		Family =	NVBLAST_FOURCC('L', 'L', 'F', 'A'),
	};
};

}	// namespace Blast
}	// namespace Nv


/**
Load all low-level serializers into the ExtSerialization manager.  *N.B.* This is done automatically when
the ExtSerialization manager is created via NvBlastExtSerializationCreate(), so currently this public function
is unnecessary.  Note also that other modules' serializers (ExtTkSerialization and ExtPxSerialization) are
_not_ loaded automatically, and need to be explicitly loaded by the user using their respective load functions.

It does no harm to call this function more than once; serializers already loaded will not be loaded again.

\param[in]	serialization	Serialization manager into which to load serializers.

\return the number of serializers loaded.
*/
NVBLAST_API	size_t		NvBlastExtLlSerializerLoadSet(Nv::Blast::ExtSerialization& serialization);


/**
Utility wrapper function to serialize an NvBlastAsset.  Allocates the buffer internally using the
callack set in ExtSerialization::setBufferProvider.

Equivalent to:

	serialization.serializeIntoBuffer(buffer, asset, Nv::Blast::LlObjectTypeID::Asset);

\param[out]	buffer			Pointer to the buffer created.
\param[in]	serialization	Serialization manager.
\param[in]	asset			Pointer to the NvBlastAsset to serialize.

\return the number of bytes serialized into the buffer (zero if unsuccessful).
*/
NVBLAST_API	uint64_t	NvBlastExtSerializationSerializeAssetIntoBuffer(void*& buffer, Nv::Blast::ExtSerialization& serialization, const NvBlastAsset* asset);


/**
Utility wrapper function to serialize an NvBlastFamily.  Allocates the buffer internally using the
callack set in ExtSerialization::setBufferProvider.

Equivalent to:

	serialization.serializeIntoBuffer(buffer, family, Nv::Blast::LlObjectTypeID::Family);

\param[out]	buffer			Pointer to the buffer created.
\param[in]	serialization	Serialization manager.
\param[in]	family			Pointer to the NvBlastFamily to serialize.

\return the number of bytes serialized into the buffer (zero if unsuccessful).
*/
NVBLAST_API	uint64_t	NvBlastExtSerializationSerializeFamilyIntoBuffer(void*& buffer, Nv::Blast::ExtSerialization& serialization, const NvBlastFamily* family);
