/*
 * Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef NVBLASTMATH_H
#define NVBLASTMATH_H

#include <math.h>

namespace Nv
{
namespace Blast
{

namespace VecMath
{


NV_INLINE void div(float a[3], float divisor)
{
	for (int i = 0; i < 3; i++)
		a[i] /= divisor;
}

NV_INLINE void mul(float a[3], float multiplier)
{
	for (int i = 0; i < 3; i++)
		a[i] *= multiplier;
}

NV_INLINE void add(const float a[3], float b[3])
{
	for (int i = 0; i < 3; i++)
		b[i] = a[i] + b[i];
}
	
NV_INLINE void add(const float a[3], const float b[3], float r[3])
{
	for (int i = 0; i < 3; i++)
		r[i] = a[i] + b[i];
}

NV_INLINE void sub(const float a[3], const float b[3], float r[3])
{
	for (int i = 0; i < 3; i++)
		r[i] = a[i] - b[i];
}

NV_INLINE float dot(const float a[3], const float b[3])
{
	float r = 0;
	for (int i = 0; i < 3; i++)
		r += a[i] * b[i];
	return r;
}

NV_INLINE float normal(const float a[3], float r[3])
{
	float length = sqrtf(dot(a, a));
	for (int i = 0; i < 3; i++)
		r[i] = a[i] / length;

	return length;
}


} // namespace VecMath

} // namespace Blast
} // namespace Nv


#endif // #ifndef NVBLASTMATH_H
