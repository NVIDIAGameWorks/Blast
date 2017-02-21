/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTTKARRAY_H
#define NVBLASTTKARRAY_H


#include "NvBlastTkAllocator.h"
#include "PsInlineArray.h"


namespace Nv
{
namespace Blast
{

template <class T>
struct TkArray
{
	typedef physx::shdfnd::Array<T, TkAllocator> type;
};


template <class T, uint32_t N>
struct TkInlineArray
{
	typedef physx::shdfnd::InlineArray<T, N, TkAllocator> type;
};

} // namespace Blast
} // namespace Nv


#endif // #ifndef NVBLASTTKARRAY_H
