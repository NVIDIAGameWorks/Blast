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


#ifndef NVBLASTTKGUID_H
#define NVBLASTTKGUID_H

#include "NvPreprocessor.h"

#if NV_WINDOWS_FAMILY
#include <rpc.h>
#else
//#include <uuid/uuid.h>
#include "NvBlastTime.h"
#endif

#include "PsHash.h"

namespace Nv
{
namespace Blast
{

#if NV_WINDOWS_FAMILY

NV_INLINE NvBlastID TkGenerateGUID(void* ptr)
{
	NV_UNUSED(ptr);

	NV_COMPILE_TIME_ASSERT(sizeof(UUID) == sizeof(NvBlastID));

	NvBlastID guid;
	UuidCreate(reinterpret_cast<UUID*>(&guid));

	return guid;
}

#else

NV_INLINE NvBlastID TkGenerateGUID(void* ptr)
{
//	NV_COMPILE_TIME_ASSERT(sizeof(uuid_t) == sizeof(NvBlastID));
	Time time;

	NvBlastID guid;
	//	uuid_generate_random(reinterpret_cast<uuid_t&>(guid));

	*reinterpret_cast<uint64_t*>(guid.data) = reinterpret_cast<uintptr_t>(ptr);
	*reinterpret_cast<int64_t*>(guid.data + 8) = time.getLastTickCount();

	return guid;
}

#endif


/**
Compares two NvBlastIDs.

\param[in]	id1	A pointer to the first id to compare.
\param[in]	id2	A pointer to the second id to compare.

\return	true iff ids are equal.
*/
NV_INLINE bool TkGUIDsEqual(const NvBlastID* id1, const NvBlastID* id2)
{
	return !memcmp(id1, id2, sizeof(NvBlastID));
}


/**
Clears an NvBlastID (sets all of its fields to zero).

\param[out]	id	A pointer to the ID to clear.
*/
NV_INLINE void TkGUIDReset(NvBlastID* id)
{
	memset(id, 0, sizeof(NvBlastID));
}


/**
Tests an NvBlastID to determine if it's zeroed.  After calling TkGUIDReset
on an ID, passing it to this function will return a value of true.

\param[in]	id	A pointer to the ID to test.
*/
NV_INLINE bool TkGUIDIsZero(const NvBlastID* id)
{
	return *reinterpret_cast<const uint64_t*>(&id->data[0]) == 0 && *reinterpret_cast<const uint64_t*>(&id->data[8]) == 0;
}

} // namespace Blast
} // namespace Nv


namespace physx
{
namespace shdfnd
{

// hash specialization for NvBlastID
template <>
struct Hash<NvBlastID>
{
	uint32_t operator()(const NvBlastID& k) const
	{
		// "DJB" string hash
		uint32_t h = 5381;
		for (uint32_t i = 0; i < sizeof(k.data) / sizeof(k.data[0]); ++i)
			h = ((h << 5) + h) ^ uint32_t(k.data[i]);
		return h;
	}
	bool equal(const NvBlastID& k0, const NvBlastID& k1) const
	{
		return Nv::Blast::TkGUIDsEqual(&k0, &k1);
	}
};

} // namespace shdfnd
} // namespace physx


#endif // #ifndef NVBLASTTKGUID_H
