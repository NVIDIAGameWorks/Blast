/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTTKHASHMAP_H
#define NVBLASTTKHASHMAP_H


#include "NvBlastTkAllocator.h"
#include "PsHashMap.h"


namespace Nv
{
namespace Blast
{

template <class Key, class Value, class HashFn = physx::shdfnd::Hash<Key>>
struct TkHashMap
{
	typedef physx::shdfnd::HashMap<Key, Value, HashFn, TkAllocator> type;
};

} // namespace Blast
} // namespace Nv


#endif // #ifndef NVBLASTTKHASHMAP_H
