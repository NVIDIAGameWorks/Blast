/*
 * Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */


#include "NvBlastAtomic.h"

#include <string.h>
#include <stdlib.h>


namespace Nv
{
namespace Blast
{


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												Windows/XBOXONE Implementation
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if NV_WINDOWS_FAMILY || NV_XBOXONE

#include "NvBlastIncludeWindows.h"

int32_t atomicIncrement(volatile int32_t* val)
{
	return (int32_t)InterlockedIncrement((volatile LONG*)val);
}

int32_t atomicDecrement(volatile int32_t* val)
{
	return (int32_t)InterlockedDecrement((volatile LONG*)val);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												Unix/PS4 Implementation
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#elif(NV_UNIX_FAMILY || NV_PS4)

int32_t atomicIncrement(volatile int32_t* val)
{
	return __sync_add_and_fetch(val, 1);
}

int32_t atomicDecrement(volatile int32_t* val)
{
	return __sync_sub_and_fetch(val, 1);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												Unsupported Platforms
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#else

#error "Platform not supported!"

#endif


} // namespace Blast
} // namespace Nv

