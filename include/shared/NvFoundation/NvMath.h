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
// Copyright (c) 2008-2014 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.

#ifndef NV_NVFOUNDATION_NVMATH_H
#define NV_NVFOUNDATION_NVMATH_H

/** \addtogroup foundation
@{
*/

#include "NvPreprocessor.h"

#if NV_VC
#pragma warning(push)
#pragma warning(disable : 4985) // 'symbol name': attributes not present on previous declaration
#endif
#include <math.h>
#if NV_VC
#pragma warning(pop)
#endif

#include <float.h>
#include "NvIntrinsics.h"
#include "NvAssert.h"

#if !NV_DOXYGEN
namespace nvidia
{
#endif

// constants
static const float NvPi = float(3.141592653589793);
static const float NvHalfPi = float(1.57079632679489661923);
static const float NvTwoPi = float(6.28318530717958647692);
static const float NvInvPi = float(0.31830988618379067154);
static const float NvInvTwoPi = float(0.15915494309189533577);
static const float NvPiDivTwo = float(1.57079632679489661923);
static const float NvPiDivFour = float(0.78539816339744830962);

/**
\brief The return value is the greater of the two specified values.
*/
template <class T>
NV_CUDA_CALLABLE NV_FORCE_INLINE T NvMax(T a, T b)
{
    return a < b ? b : a;
}

//! overload for float to use fsel on xbox
template <>
NV_CUDA_CALLABLE NV_FORCE_INLINE float NvMax(float a, float b)
{
    return intrinsics::selectMax(a, b);
}

/**
\brief The return value is the lesser of the two specified values.
*/
template <class T>
NV_CUDA_CALLABLE NV_FORCE_INLINE T NvMin(T a, T b)
{
    return a < b ? a : b;
}

template <>
//! overload for float to use fsel on xbox
NV_CUDA_CALLABLE NV_FORCE_INLINE float NvMin(float a, float b)
{
    return intrinsics::selectMin(a, b);
}

/*
Many of these are just implemented as NV_CUDA_CALLABLE NV_FORCE_INLINE calls to the C lib right now,
but later we could replace some of them with some approximations or more
clever stuff.
*/

/**
\brief abs returns the absolute value of its argument.
*/
NV_CUDA_CALLABLE NV_FORCE_INLINE float NvAbs(float a)
{
    return intrinsics::abs(a);
}

NV_CUDA_CALLABLE NV_FORCE_INLINE bool NvEquals(float a, float b, float eps)
{
    return (NvAbs(a - b) < eps);
}

/**
\brief abs returns the absolute value of its argument.
*/
NV_CUDA_CALLABLE NV_FORCE_INLINE double NvAbs(double a)
{
    return ::fabs(a);
}

/**
\brief abs returns the absolute value of its argument.
*/
NV_CUDA_CALLABLE NV_FORCE_INLINE int32_t NvAbs(int32_t a)
{
    return ::abs(a);
}

/**
\brief Clamps v to the range [hi,lo]
*/
template <class T>
NV_CUDA_CALLABLE NV_FORCE_INLINE T NvClamp(T v, T lo, T hi)
{
    NV_ASSERT(lo <= hi);
    return NvMin(hi, NvMax(lo, v));
}

//! \brief Square root.
NV_CUDA_CALLABLE NV_FORCE_INLINE float NvSqrt(float a)
{
    return intrinsics::sqrt(a);
}

//! \brief Square root.
NV_CUDA_CALLABLE NV_FORCE_INLINE double NvSqrt(double a)
{
    return ::sqrt(a);
}

//! \brief reciprocal square root.
NV_CUDA_CALLABLE NV_FORCE_INLINE float NvRecipSqrt(float a)
{
    return intrinsics::recipSqrt(a);
}

//! \brief reciprocal square root.
NV_CUDA_CALLABLE NV_FORCE_INLINE double NvRecipSqrt(double a)
{
    return 1 / ::sqrt(a);
}

//! trigonometry -- all angles are in radians.

//! \brief Sine of an angle ( <b>Unit:</b> Radians )
NV_CUDA_CALLABLE NV_FORCE_INLINE float NvSin(float a)
{
    return intrinsics::sin(a);
}

//! \brief Sine of an angle ( <b>Unit:</b> Radians )
NV_CUDA_CALLABLE NV_FORCE_INLINE double NvSin(double a)
{
    return ::sin(a);
}

//! \brief Cosine of an angle (<b>Unit:</b> Radians)
NV_CUDA_CALLABLE NV_FORCE_INLINE float NvCos(float a)
{
    return intrinsics::cos(a);
}

//! \brief Cosine of an angle (<b>Unit:</b> Radians)
NV_CUDA_CALLABLE NV_FORCE_INLINE double NvCos(double a)
{
    return ::cos(a);
}

/**
\brief Tangent of an angle.
<b>Unit:</b> Radians
*/
NV_CUDA_CALLABLE NV_FORCE_INLINE float NvTan(float a)
{
    return ::tanf(a);
}

/**
\brief Tangent of an angle.
<b>Unit:</b> Radians
*/
NV_CUDA_CALLABLE NV_FORCE_INLINE double NvTan(double a)
{
    return ::tan(a);
}

/**
\brief Arcsine.
Returns angle between -PI/2 and PI/2 in radians
<b>Unit:</b> Radians
*/
NV_CUDA_CALLABLE NV_FORCE_INLINE float NvAsin(float f)
{
    return ::asinf(NvClamp(f, -1.0f, 1.0f));
}

/**
\brief Arcsine.
Returns angle between -PI/2 and PI/2 in radians
<b>Unit:</b> Radians
*/
NV_CUDA_CALLABLE NV_FORCE_INLINE double NvAsin(double f)
{
    return ::asin(NvClamp(f, -1.0, 1.0));
}

/**
\brief Arccosine.
Returns angle between 0 and PI in radians
<b>Unit:</b> Radians
*/
NV_CUDA_CALLABLE NV_FORCE_INLINE float NvAcos(float f)
{
    return ::acosf(NvClamp(f, -1.0f, 1.0f));
}

/**
\brief Arccosine.
Returns angle between 0 and PI in radians
<b>Unit:</b> Radians
*/
NV_CUDA_CALLABLE NV_FORCE_INLINE double NvAcos(double f)
{
    return ::acos(NvClamp(f, -1.0, 1.0));
}

/**
\brief ArcTangent.
Returns angle between -PI/2 and PI/2 in radians
<b>Unit:</b> Radians
*/
NV_CUDA_CALLABLE NV_FORCE_INLINE float NvAtan(float a)
{
    return ::atanf(a);
}

/**
\brief ArcTangent.
Returns angle between -PI/2 and PI/2 in radians
<b>Unit:</b> Radians
*/
NV_CUDA_CALLABLE NV_FORCE_INLINE double NvAtan(double a)
{
    return ::atan(a);
}

/**
\brief Arctangent of (x/y) with correct sign.
Returns angle between -PI and PI in radians
<b>Unit:</b> Radians
*/
NV_CUDA_CALLABLE NV_FORCE_INLINE float NvAtan2(float x, float y)
{
    return ::atan2f(x, y);
}

/**
\brief Arctangent of (x/y) with correct sign.
Returns angle between -PI and PI in radians
<b>Unit:</b> Radians
*/
NV_CUDA_CALLABLE NV_FORCE_INLINE double NvAtan2(double x, double y)
{
    return ::atan2(x, y);
}

//! \brief returns true if the passed number is a finite floating point number as opposed to INF, NAN, etc.
NV_CUDA_CALLABLE NV_FORCE_INLINE bool NvIsFinite(float f)
{
    return intrinsics::isFinite(f);
}

//! \brief returns true if the passed number is a finite floating point number as opposed to INF, NAN, etc.
NV_CUDA_CALLABLE NV_FORCE_INLINE bool NvIsFinite(double f)
{
    return intrinsics::isFinite(f);
}

NV_CUDA_CALLABLE NV_FORCE_INLINE float NvFloor(float a)
{
    return ::floorf(a);
}

NV_CUDA_CALLABLE NV_FORCE_INLINE float NvExp(float a)
{
    return ::expf(a);
}

NV_CUDA_CALLABLE NV_FORCE_INLINE float NvCeil(float a)
{
    return ::ceilf(a);
}

NV_CUDA_CALLABLE NV_FORCE_INLINE float NvSign(float a)
{
    return nvidia::intrinsics::sign(a);
}

NV_CUDA_CALLABLE NV_FORCE_INLINE float NvPow(float x, float y)
{
    return ::powf(x, y);
}

NV_CUDA_CALLABLE NV_FORCE_INLINE float NvLog(float x)
{
    return ::logf(x);
}

#if !NV_DOXYGEN
} // namespace nvidia
#endif

/** @} */
#endif // #ifndef NV_NVFOUNDATION_NVMATH_H
