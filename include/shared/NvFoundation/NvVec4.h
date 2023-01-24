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

#ifndef NV_NVFOUNDATION_NVVEC4_H
#define NV_NVFOUNDATION_NVVEC4_H
/** \addtogroup foundation
@{
*/
#include "NvMath.h"
#include "NvVec3.h"
#include "NvAssert.h"

/**
\brief 4 Element vector class.

This is a 4-dimensional vector class with public data members.
*/
#if !NV_DOXYGEN
namespace nvidia
{
#endif

class NvVec4
{
  public:
    /**
    \brief default constructor leaves data uninitialized.
    */
    NV_CUDA_CALLABLE NV_INLINE NvVec4()
    {
    }

    /**
    \brief zero constructor.
    */
    NV_CUDA_CALLABLE NV_FORCE_INLINE NvVec4(NvZERO r) : x(0.0f), y(0.0f), z(0.0f), w(0.0f)
    {
        NV_UNUSED(r);
    }

    /**
    \brief Assigns scalar parameter to all elements.

    Useful to initialize to zero or one.

    \param[in] a Value to assign to elements.
    */
    explicit NV_CUDA_CALLABLE NV_INLINE NvVec4(float a) : x(a), y(a), z(a), w(a)
    {
    }

    /**
    \brief Initializes from 3 scalar parameters.

    \param[in] nx Value to initialize X component.
    \param[in] ny Value to initialize Y component.
    \param[in] nz Value to initialize Z component.
    \param[in] nw Value to initialize W component.
    */
    NV_CUDA_CALLABLE NV_INLINE NvVec4(float nx, float ny, float nz, float nw) : x(nx), y(ny), z(nz), w(nw)
    {
    }

    /**
    \brief Initializes from 3 scalar parameters.

    \param[in] v Value to initialize the X, Y, and Z components.
    \param[in] nw Value to initialize W component.
    */
    NV_CUDA_CALLABLE NV_INLINE NvVec4(const NvVec3& v, float nw) : x(v.x), y(v.y), z(v.z), w(nw)
    {
    }

    /**
    \brief Initializes from an array of scalar parameters.

    \param[in] v Value to initialize with.
    */
    explicit NV_CUDA_CALLABLE NV_INLINE NvVec4(const float v[]) : x(v[0]), y(v[1]), z(v[2]), w(v[3])
    {
    }

    /**
    \brief Copy ctor.
    */
    NV_CUDA_CALLABLE NV_INLINE NvVec4(const NvVec4& v) : x(v.x), y(v.y), z(v.z), w(v.w)
    {
    }

    // Operators

    /**
    \brief Assignment operator
    */
    NV_CUDA_CALLABLE NV_INLINE NvVec4& operator=(const NvVec4& p)
    {
        x = p.x;
        y = p.y;
        z = p.z;
        w = p.w;
        return *this;
    }

    /**
    \brief element access
    */
    NV_DEPRECATED NV_CUDA_CALLABLE NV_INLINE float& operator[](unsigned int index)
    {
        NV_ASSERT(index <= 3);

        return reinterpret_cast<float*>(this)[index];
    }

    /**
    \brief element access
    */
    NV_DEPRECATED NV_CUDA_CALLABLE NV_INLINE const float& operator[](unsigned int index) const
    {
        NV_ASSERT(index <= 3);

        return reinterpret_cast<const float*>(this)[index];
    }

    /**
    \brief returns true if the two vectors are exactly equal.
    */
    NV_CUDA_CALLABLE NV_INLINE bool operator==(const NvVec4& v) const
    {
        return x == v.x && y == v.y && z == v.z && w == v.w;
    }

    /**
    \brief returns true if the two vectors are not exactly equal.
    */
    NV_CUDA_CALLABLE NV_INLINE bool operator!=(const NvVec4& v) const
    {
        return x != v.x || y != v.y || z != v.z || w != v.w;
    }

    /**
    \brief tests for exact zero vector
    */
    NV_CUDA_CALLABLE NV_INLINE bool isZero() const
    {
        return x == 0 && y == 0 && z == 0 && w == 0;
    }

    /**
    \brief returns true if all 3 elems of the vector are finite (not NAN or INF, etc.)
    */
    NV_CUDA_CALLABLE NV_INLINE bool isFinite() const
    {
        return NvIsFinite(x) && NvIsFinite(y) && NvIsFinite(z) && NvIsFinite(w);
    }

    /**
    \brief is normalized - used by API parameter validation
    */
    NV_CUDA_CALLABLE NV_INLINE bool isNormalized() const
    {
        const float unitTolerance = 1e-4f;
        return isFinite() && NvAbs(magnitude() - 1) < unitTolerance;
    }

    /**
    \brief returns the squared magnitude

    Avoids calling NvSqrt()!
    */
    NV_CUDA_CALLABLE NV_INLINE float magnitudeSquared() const
    {
        return x * x + y * y + z * z + w * w;
    }

    /**
    \brief returns the magnitude
    */
    NV_CUDA_CALLABLE NV_INLINE float magnitude() const
    {
        return NvSqrt(magnitudeSquared());
    }

    /**
    \brief negation
    */
    NV_CUDA_CALLABLE NV_INLINE NvVec4 operator-() const
    {
        return NvVec4(-x, -y, -z, -w);
    }

    /**
    \brief vector addition
    */
    NV_CUDA_CALLABLE NV_INLINE NvVec4 operator+(const NvVec4& v) const
    {
        return NvVec4(x + v.x, y + v.y, z + v.z, w + v.w);
    }

    /**
    \brief vector difference
    */
    NV_CUDA_CALLABLE NV_INLINE NvVec4 operator-(const NvVec4& v) const
    {
        return NvVec4(x - v.x, y - v.y, z - v.z, w - v.w);
    }

    /**
    \brief scalar post-multiplication
    */

    NV_CUDA_CALLABLE NV_INLINE NvVec4 operator*(float f) const
    {
        return NvVec4(x * f, y * f, z * f, w * f);
    }

    /**
    \brief scalar division
    */
    NV_CUDA_CALLABLE NV_INLINE NvVec4 operator/(float f) const
    {
        f = 1.0f / f;
        return NvVec4(x * f, y * f, z * f, w * f);
    }

    /**
    \brief vector addition
    */
    NV_CUDA_CALLABLE NV_INLINE NvVec4& operator+=(const NvVec4& v)
    {
        x += v.x;
        y += v.y;
        z += v.z;
        w += v.w;
        return *this;
    }

    /**
    \brief vector difference
    */
    NV_CUDA_CALLABLE NV_INLINE NvVec4& operator-=(const NvVec4& v)
    {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        w -= v.w;
        return *this;
    }

    /**
    \brief scalar multiplication
    */
    NV_CUDA_CALLABLE NV_INLINE NvVec4& operator*=(float f)
    {
        x *= f;
        y *= f;
        z *= f;
        w *= f;
        return *this;
    }
    /**
    \brief scalar division
    */
    NV_CUDA_CALLABLE NV_INLINE NvVec4& operator/=(float f)
    {
        f = 1.0f / f;
        x *= f;
        y *= f;
        z *= f;
        w *= f;
        return *this;
    }

    /**
    \brief returns the scalar product of this and other.
    */
    NV_CUDA_CALLABLE NV_INLINE float dot(const NvVec4& v) const
    {
        return x * v.x + y * v.y + z * v.z + w * v.w;
    }

    /** return a unit vector */

    NV_CUDA_CALLABLE NV_INLINE NvVec4 getNormalized() const
    {
        float m = magnitudeSquared();
        return m > 0.0f ? *this * NvRecipSqrt(m) : NvVec4(0, 0, 0, 0);
    }

    /**
    \brief normalizes the vector in place
    */
    NV_CUDA_CALLABLE NV_INLINE float normalize()
    {
        float m = magnitude();
        if(m > 0.0f)
            *this /= m;
        return m;
    }

    /**
    \brief a[i] * b[i], for all i.
    */
    NV_CUDA_CALLABLE NV_INLINE NvVec4 multiply(const NvVec4& a) const
    {
        return NvVec4(x * a.x, y * a.y, z * a.z, w * a.w);
    }

    /**
    \brief element-wise minimum
    */
    NV_CUDA_CALLABLE NV_INLINE NvVec4 minimum(const NvVec4& v) const
    {
        return NvVec4(NvMin(x, v.x), NvMin(y, v.y), NvMin(z, v.z), NvMin(w, v.w));
    }

    /**
    \brief element-wise maximum
    */
    NV_CUDA_CALLABLE NV_INLINE NvVec4 maximum(const NvVec4& v) const
    {
        return NvVec4(NvMax(x, v.x), NvMax(y, v.y), NvMax(z, v.z), NvMax(w, v.w));
    }

    NV_CUDA_CALLABLE NV_INLINE NvVec3 getXYZ() const
    {
        return NvVec3(x, y, z);
    }

    /**
    \brief set vector elements to zero
    */
    NV_CUDA_CALLABLE NV_INLINE void setZero()
    {
        x = y = z = w = 0.0f;
    }

    float x, y, z, w;
};

NV_CUDA_CALLABLE static NV_INLINE NvVec4 operator*(float f, const NvVec4& v)
{
    return NvVec4(f * v.x, f * v.y, f * v.z, f * v.w);
}

#if !NV_DOXYGEN
} // namespace nvidia
#endif

/** @} */
#endif // #ifndef NV_NVFOUNDATION_NVVEC4_H
