/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTTKHASHSET_H
#define NVBLASTTKHASHSET_H


#include "NvBlastTkAllocator.h"
#include "PsHashSet.h"


namespace Nv
{
namespace Blast
{

template <class Key, class HashFn = physx::shdfnd::Hash<Key>>
struct TkHashSet
{
	typedef physx::shdfnd::HashSet<Key, HashFn, TkAllocator> type;
};

} // namespace Blast
} // namespace Nv


#endif // #ifndef NVBLASTTKHASHSET_H
