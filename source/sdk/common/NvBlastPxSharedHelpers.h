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


#ifndef NVBLASTPXSHAREDTYPESHELPERS_H
#define NVBLASTPXSHAREDTYPESHELPERS_H

#include "NvCTypes.h"
#include <foundation/PxVec2.h>
#include <foundation/PxVec3.h>
#include <foundation/PxVec4.h>
#include <foundation/PxTransform.h>
#include <foundation/PxPlane.h>
#include <foundation/PxMat33.h>
#include <foundation/PxMat44.h>
#include <foundation/PxBounds3.h>

#define WCast(type, name) reinterpret_cast<type>(name)
#define RCast(type, name) reinterpret_cast<const type>(name)

#define CONVERT(BlastType, PxSharedType)                                                                                  \
	static inline PxSharedType& toPxShared(BlastType& v)                                                                     \
	{                                                                                                                  \
		return WCast(PxSharedType&, v);                                                                                   \
	}                                                                                                                  \
	static inline const PxSharedType& toPxShared(const BlastType& v)                                                         \
	{                                                                                                                  \
		return RCast(PxSharedType&, v);                                                                                   \
	}                                                                                                                  \
	static inline const BlastType& fromPxShared(const PxSharedType& v)                                                       \
	{                                                                                                                  \
		return RCast(BlastType&, v);                                                                                   \
	}                                                                                                                  \
	static inline BlastType& fromPxShared(PxSharedType& v)                                                                   \
	{                                                                                                                  \
		return WCast(BlastType&, v);                                                                                   \
	}                                                                                                                  \
	static inline PxSharedType* toPxShared(BlastType* v)                                                                     \
	{                                                                                                                  \
		return WCast(PxSharedType*, v);                                                                                   \
	}                                                                                                                  \
	static inline const PxSharedType* toPxShared(const BlastType* v)                                                         \
	{                                                                                                                  \
		return RCast(PxSharedType*, v);                                                                                   \
	}                                                                                                                  \
	static inline const BlastType* fromPxShared(const PxSharedType* v)                                                       \
	{                                                                                                                  \
		return RCast(BlastType*, v);                                                                                   \
	}                                                                                                                  \
	static inline BlastType* fromPxShared(PxSharedType* v)                                                                   \
	{                                                                                                                  \
		return WCast(BlastType*, v);                                                                                   \
	}


CONVERT(NvcVec2, physx::PxVec2)
CONVERT(NvcVec3, physx::PxVec3)
CONVERT(NvcVec4, physx::PxVec4)
CONVERT(NvcQuat, physx::PxQuat)
CONVERT(NvcTransform, physx::PxTransform)
CONVERT(NvcPlane, physx::PxPlane)
CONVERT(NvcMat33, physx::PxMat33)
CONVERT(NvcMat44, physx::PxMat44)
CONVERT(NvcBounds3, physx::PxBounds3)

NV_COMPILE_TIME_ASSERT(sizeof(NvcVec2) == sizeof(physx::PxVec2));
NV_COMPILE_TIME_ASSERT(NV_OFFSET_OF(NvcVec2, x) == NV_OFFSET_OF(physx::PxVec2, x));
NV_COMPILE_TIME_ASSERT(NV_OFFSET_OF(NvcVec2, y) == NV_OFFSET_OF(physx::PxVec2, y));

NV_COMPILE_TIME_ASSERT(sizeof(NvcVec3) == sizeof(physx::PxVec3));
NV_COMPILE_TIME_ASSERT(NV_OFFSET_OF(NvcVec3, x) == NV_OFFSET_OF(physx::PxVec3, x));
NV_COMPILE_TIME_ASSERT(NV_OFFSET_OF(NvcVec3, y) == NV_OFFSET_OF(physx::PxVec3, y));
NV_COMPILE_TIME_ASSERT(NV_OFFSET_OF(NvcVec3, z) == NV_OFFSET_OF(physx::PxVec3, z));

NV_COMPILE_TIME_ASSERT(sizeof(NvcVec4) == sizeof(physx::PxVec4));
NV_COMPILE_TIME_ASSERT(NV_OFFSET_OF(NvcVec4, x) == NV_OFFSET_OF(physx::PxVec4, x));
NV_COMPILE_TIME_ASSERT(NV_OFFSET_OF(NvcVec4, y) == NV_OFFSET_OF(physx::PxVec4, y));
NV_COMPILE_TIME_ASSERT(NV_OFFSET_OF(NvcVec4, z) == NV_OFFSET_OF(physx::PxVec4, z));
NV_COMPILE_TIME_ASSERT(NV_OFFSET_OF(NvcVec4, w) == NV_OFFSET_OF(physx::PxVec4, w));

NV_COMPILE_TIME_ASSERT(sizeof(NvcQuat) == sizeof(physx::PxQuat));
NV_COMPILE_TIME_ASSERT(NV_OFFSET_OF(NvcQuat, x) == NV_OFFSET_OF(physx::PxQuat, x));
NV_COMPILE_TIME_ASSERT(NV_OFFSET_OF(NvcQuat, y) == NV_OFFSET_OF(physx::PxQuat, y));
NV_COMPILE_TIME_ASSERT(NV_OFFSET_OF(NvcQuat, z) == NV_OFFSET_OF(physx::PxQuat, z));
NV_COMPILE_TIME_ASSERT(NV_OFFSET_OF(NvcQuat, w) == NV_OFFSET_OF(physx::PxQuat, w));

NV_COMPILE_TIME_ASSERT(sizeof(NvcTransform) == sizeof(physx::PxTransform));
NV_COMPILE_TIME_ASSERT(NV_OFFSET_OF(NvcTransform, p) == NV_OFFSET_OF(physx::PxTransform, p));
NV_COMPILE_TIME_ASSERT(NV_OFFSET_OF(NvcTransform, q) == NV_OFFSET_OF(physx::PxTransform, q));

NV_COMPILE_TIME_ASSERT(sizeof(NvcPlane) == sizeof(physx::PxPlane));
NV_COMPILE_TIME_ASSERT(NV_OFFSET_OF(NvcPlane, n) == NV_OFFSET_OF(physx::PxPlane, n));
NV_COMPILE_TIME_ASSERT(NV_OFFSET_OF(NvcPlane, d) == NV_OFFSET_OF(physx::PxPlane, d));

NV_COMPILE_TIME_ASSERT(sizeof(NvcMat33) == sizeof(physx::PxMat33));
NV_COMPILE_TIME_ASSERT(NV_OFFSET_OF(NvcMat33, column0) == NV_OFFSET_OF(physx::PxMat33, column0));
NV_COMPILE_TIME_ASSERT(NV_OFFSET_OF(NvcMat33, column1) == NV_OFFSET_OF(physx::PxMat33, column1));
NV_COMPILE_TIME_ASSERT(NV_OFFSET_OF(NvcMat33, column2) == NV_OFFSET_OF(physx::PxMat33, column2));

NV_COMPILE_TIME_ASSERT(sizeof(NvcBounds3) == sizeof(physx::PxBounds3));
NV_COMPILE_TIME_ASSERT(NV_OFFSET_OF(NvcBounds3, minimum) == NV_OFFSET_OF(physx::PxBounds3, minimum));
NV_COMPILE_TIME_ASSERT(NV_OFFSET_OF(NvcBounds3, maximum) == NV_OFFSET_OF(physx::PxBounds3, maximum));

// Some basic operators
inline NvcVec2 operator+(const NvcVec2& v1, const NvcVec2& v2)
{
	return{ v1.x + v2.x, v1.y + v2.y };
}
inline NvcVec2 operator-(const NvcVec2& v1, const NvcVec2& v2)
{
	return{ v1.x - v2.x, v1.y - v2.y };
}
inline NvcVec2 operator+(const NvcVec2& v, float f)
{
	return{ v.x + f, v.y + f };
}
inline NvcVec2 operator+(float f, const NvcVec2& v)
{
	return{ v.x + f, v.y + f };
}
inline NvcVec2 operator*(const NvcVec2& v, float f)
{
	return{ v.x * f, v.y * f };
}
inline NvcVec2 operator*(float f, const NvcVec2& v)
{
	return{ v.x * f, v.y * f };
}
inline NvcVec2 operator/(const NvcVec2& v, float f)
{
	return{ v.x / f, v.y / f };
}
inline float dot(const NvcVec2& v1, const NvcVec2& v2)
{
	return v1.x * v2.x + v1.y * v2.y;
}
inline NvcVec2 neg(const NvcVec2& v)
{
	return{ -v.x, -v.y };
}

inline NvcVec3 operator+(const NvcVec3& v1, const NvcVec3& v2)
{
	return{ v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
}
inline NvcVec3 operator-(const NvcVec3& v1, const NvcVec3& v2)
{
	return{ v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
}
inline NvcVec3 operator*(const NvcVec3& v1, const NvcVec3& v2)
{
	return{ v1.x * v2.x, v1.y * v2.y, v1.z * v2.z };
}
inline NvcVec3 operator/(const NvcVec3& v1, const NvcVec3& v2)
{
	return{ v1.x / v2.x, v1.y / v2.y, v1.z / v2.z };
}
inline NvcVec3 operator+(const NvcVec3& v, float f)
{
	return{ v.x + f, v.y + f, v.z + f };
}
inline NvcVec3 operator+(float f, const NvcVec3& v)
{
	return{ v.x + f, v.y + f, v.z + f };
}
inline NvcVec3 operator*(const NvcVec3& v, float f)
{
	return{ v.x * f, v.y * f, v.z * f };
}
inline NvcVec3 operator*(float f, const NvcVec3& v)
{
	return{ v.x * f, v.y * f, v.z * f };
}
inline NvcVec3 operator/(const NvcVec3& v, float f)
{
	return{ v.x / f, v.y / f, v.z / f };
}
inline float dot(const NvcVec3& v1, const NvcVec3& v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}
inline NvcVec3 neg(const NvcVec3& v)
{
	return{ -v.x, -v.y, -v.z };
}

#endif  // #ifndef NVBLASTPHYSXTYPESHELPERS_H
