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


#ifndef NVBLASTMEMORY_H
#define NVBLASTMEMORY_H

#include <math.h>

namespace Nv
{
namespace Blast
{


/**
Utility function to align the given value to the next 16-byte boundary.

Returns the aligned value.
*/
template<typename T>
NV_INLINE T align16(T value)
{
	return (value + 0xF)&~(T)0xF;
}


/** Offset void* pointer by 'offset' bytes helper-functions */

template <typename T>
NV_INLINE T pointerOffset(void* p, ptrdiff_t offset)
{
	return reinterpret_cast<T>(reinterpret_cast<char*>(p)+offset);
}

template <typename T>
NV_INLINE T pointerOffset(const void* p, ptrdiff_t offset)
{
	return reinterpret_cast<T>(reinterpret_cast<const char*>(p)+offset);
}

NV_INLINE const void* pointerOffset(const void* p, ptrdiff_t offset)
{
	return pointerOffset<const void*>(p, offset);
}

NV_INLINE void* pointerOffset(void* p, ptrdiff_t offset)
{
	return pointerOffset<void*>(p, offset);
}

} // namespace Blast
} // namespace Nv


/** Block data offset and accessor macro. */
#define NvBlastBlockData(_dataType, _name, _accessor) \
_dataType* _accessor() const \
{ \
	return (_dataType*)((uintptr_t)this + _name); \
} \
uint32_t _name


/** Block data offset and accessor macro for an array (includes an _accessor##ArraySize() function which returns the last expression). */
#define NvBlastBlockArrayData(_dataType, _name, _accessor, _sizeExpr) \
_dataType* _accessor() const \
{ \
	return (_dataType*)((uintptr_t)this + _name); \
} \
uint32_t _accessor##ArraySize() const \
{ \
	return _sizeExpr; \
} \
uint32_t _name


/** Block data offset generation macros. */

/** Start offset generation with this. */
#define NvBlastCreateOffsetStart(_baseOffset) \
size_t _lastOffset = _baseOffset; \
size_t _lastSize = 0

/** Create the next offset generation with this.  The value will be aligned to a 16-byte boundary. */
#define NvBlastCreateOffsetAlign16(_name, _size) \
_name = align16(_lastOffset + _lastSize); \
_lastOffset = _name; \
_lastSize = _size

/** End offset generation with this.  It evaluates to the (16-byte aligned) total size of the data block. */
#define NvBlastCreateOffsetEndAlign16() \
align16(_lastOffset + _lastSize)


/** Stack allocation */
#if NV_WINDOWS_FAMILY
#include <malloc.h>
#define NvBlastAlloca(x) _alloca(x)
#elif NV_LINUX || NV_ANDROID
#include <alloca.h>
#define NvBlastAlloca(x) alloca(x)
#elif NV_APPLE_FAMILY
#include <alloca.h>
#define NvBlastAlloca(x) alloca(x)
#elif NV_PS4
#include <memory.h>
#define NvBlastAlloca(x) alloca(x)
#elif NV_XBOXONE
#include <malloc.h>
#define NvBlastAlloca(x) alloca(x)
#endif

#define NvBlastAllocaAligned16(x) (void*)(((uintptr_t)PxAlloca(x + 0xF) + 0xF) & ~(uintptr_t)0xF)


#endif // #ifndef NVBLASTMEMORY_H
