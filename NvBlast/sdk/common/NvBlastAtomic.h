/*
 * Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef NVBLASTATOMIC_H
#define NVBLASTATOMIC_H

#include "NvBlastTypes.h"


namespace Nv
{
namespace Blast
{

/* increment the specified location. Return the incremented value */
int32_t atomicIncrement(volatile int32_t* val);


/* decrement the specified location. Return the decremented value */
int32_t atomicDecrement(volatile int32_t* val);

} // namespace Blast
} // namespace Nv

#endif // #ifndef NVBLASTATOMIC_H
