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
// Copyright (c) 2016-2020 NVIDIA Corporation. All rights reserved.


#ifndef NVCMATH_H
#define NVCMATH_H

#include "NvCTypes.h"

/**
 * Some basic operators for NvcVec2 and NvcVec3
 */


/* NvcVec2 operators */

// Vector sum
inline NvcVec2 operator + (const NvcVec2& v, const NvcVec2& w)
{
    return { v.x + w.x, v.y + w.y };
}

// Vector difference
inline NvcVec2 operator - (const NvcVec2& v, const NvcVec2& w)
{
    return { v.x - w.x, v.y - w.y };
}

// Vector component product
inline NvcVec2 operator * (const NvcVec2& v, const NvcVec2& w)
{
    return { v.x * w.x, v.y * w.y };
}

// Vector component quotient
inline NvcVec2 operator / (const NvcVec2& v, const NvcVec2& w)
{
    return { v.x / w.x, v.y / w.y };
}

// Vector product with scalar (on right)
inline NvcVec2 operator * (const NvcVec2& v, float f)
{
    return { v.x * f, v.y * f };
}

// Vector product with scalar (on left)
inline NvcVec2 operator * (float f, const NvcVec2& v)
{
    return { f * v.x, f * v.y };
}

// Vector quotient with scalar (on right)
inline NvcVec2 operator / (const NvcVec2& v, float f)
{
    return { v.x / f, v.y / f };
}

// Vector quotient with scalar (on left)
inline NvcVec2 operator / (float f, const NvcVec2& v)
{
    return { f / v.x, f / v.y };
}

// Inner product
inline float operator | (const NvcVec2& v, const NvcVec2& w)
{
    return v.x * w.x + v.y * w.y;
}

// Vector negation
inline NvcVec2 operator - (const NvcVec2& v)
{
    return { -v.x, -v.y };
}

/* NvcVec2 assignment operators */

// Vector sum with assignment
inline NvcVec2& operator += (NvcVec2& v, const NvcVec2& w)
{
    return v = v + w;
}

// Vector difference with assignment
inline NvcVec2& operator -= (NvcVec2& v, const NvcVec2& w)
{
    return v = v - w;
}

// Vector component product with assignment
inline NvcVec2& operator *= (NvcVec2& v, const NvcVec2& w)
{
    return v = v * w;
}

// Vector component quotient with assignment
inline NvcVec2& operator /= (NvcVec2& v, const NvcVec2& w)
{
    return v = v / w;
}

// Vector product with scalar with assignment
inline NvcVec2& operator *= (NvcVec2& v, float f)
{
    return v = v * f;
}

// Vector quotient with scalar with assignment
inline NvcVec2& operator /= (NvcVec2& v, float f)
{
    return v = v / f;
}


/* NvcVec3 operators */

// Vector sum
inline NvcVec3 operator + (const NvcVec3& v, const NvcVec3& w)
{
    return { v.x + w.x, v.y + w.y, v.z + w.z };
}

// Vector difference
inline NvcVec3 operator - (const NvcVec3& v, const NvcVec3& w)
{
    return { v.x - w.x, v.y - w.y, v.z - w.z };
}

// Vector component product
inline NvcVec3 operator * (const NvcVec3& v, const NvcVec3& w)
{
    return { v.x * w.x, v.y * w.y, v.z * w.z };
}

// Vector component quotient
inline NvcVec3 operator / (const NvcVec3& v, const NvcVec3& w)
{
    return { v.x / w.x, v.y / w.y, v.z / w.z };
}

// Vector product with scalar (on right)
inline NvcVec3 operator * (const NvcVec3& v, float f)
{
    return { v.x * f, v.y * f, v.z * f };
}

// Vector product with scalar (on left)
inline NvcVec3 operator * (float f, const NvcVec3& v)
{
    return { f * v.x, f * v.y, f * v.z };
}

// Vector quotient with scalar (on right)
inline NvcVec3 operator / (const NvcVec3& v, float f)
{
    return { v.x / f, v.y / f, v.z / f };
}

// Vector quotient with scalar (on left)
inline NvcVec3 operator / (float f, const NvcVec3& v)
{
    return { f / v.x, f / v.y, f / v.z };
}

// Inner product
inline float operator | (const NvcVec3& v, const NvcVec3& w)
{
    return v.x * w.x + v.y * w.y + v.z * w.z;
}

// Cross product
inline NvcVec3 operator ^ (const NvcVec3& v, const NvcVec3& w)
{
    return { v.x * w.y - v.y * w.x, v.z * w.x - v.x * w.z, v.y * w.z - v.z * w.y };
}

// Vector negation
inline NvcVec3 operator - (const NvcVec3& v)
{
    return { -v.x, -v.y, -v.z };
}

/* NvcVec3 assignment operators */

// Vector sum with assignment
inline NvcVec3& operator += (NvcVec3& v, const NvcVec3& w)
{
    return v = v + w;
}

// Vector difference with assignment
inline NvcVec3& operator -= (NvcVec3& v, const NvcVec3& w)
{
    return v = v - w;
}

// Vector component product with assignment
inline NvcVec3& operator *= (NvcVec3& v, const NvcVec3& w)
{
    return v = v * w;
}

// Vector component quotient with assignment
inline NvcVec3& operator /= (NvcVec3& v, const NvcVec3& w)
{
    return v = v / w;
}

// Vector product with scalar with assignment
inline NvcVec3& operator *= (NvcVec3& v, float f)
{
    return v = v * f;
}

// Vector quotient with scalar with assignment
inline NvcVec3& operator /= (NvcVec3& v, float f)
{
    return v = v / f;
}

#endif  // #ifndef NVCMATH_H
