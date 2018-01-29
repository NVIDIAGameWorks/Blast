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


#ifndef NVBLASTARRAY_H
#define NVBLASTARRAY_H


#include "NvBlastAllocator.h"
#include "PsInlineArray.h"


namespace Nv
{
namespace Blast
{

/**
Wrapped PxShared Array that uses NvBlastGlobals AllocatorCalllback.
*/
template <class T>
struct Array
{
	typedef physx::shdfnd::Array<T, Allocator> type;
};


/**
Wrapped PxShared InlineArray that uses NvBlastGlobals AllocatorCalllback.

InlineArraya is array that pre-allocates for N elements.
*/
template <class T, uint32_t N>
struct InlineArray
{
	typedef physx::shdfnd::InlineArray<T, N, Allocator> type;
};

} // namespace Blast
} // namespace Nv


#endif // #ifndef NVBLASTARRAY_H
