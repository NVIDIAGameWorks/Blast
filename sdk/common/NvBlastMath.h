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

NV_INLINE float length(const float a[3])
{
	return sqrtf(dot(a, a));
}

NV_INLINE float dist(const float a[3], const float b[3])
{
	float v[3];
	sub(a, b, v);
	return length(v);
}

NV_INLINE float normal(const float a[3], float r[3])
{
	float d = length(a);
	for (int i = 0; i < 3; i++)
		r[i] = a[i] / d;

	return d;
}


} // namespace VecMath

} // namespace Blast
} // namespace Nv


#endif // #ifndef NVBLASTMATH_H
