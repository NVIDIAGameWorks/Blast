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

#ifndef NV_NVFOUNDATION_NVQUAT_H
#define NV_NVFOUNDATION_NVQUAT_H

/** \addtogroup foundation
@{
*/

#include "NvVec3.h"
#if !NV_DOXYGEN
namespace nvidia
{
#endif

/**
\brief This is a quaternion class. For more information on quaternion mathematics
consult a mathematics source on complex numbers.

*/

class NvQuat
{
  public:
    /**
    \brief Default constructor, does not do any initialization.
    */
    NV_CUDA_CALLABLE NV_FORCE_INLINE NvQuat()
    {
    }

    //! identity constructor
    NV_CUDA_CALLABLE NV_INLINE NvQuat(NvIDENTITY r) : x(0.0f), y(0.0f), z(0.0f), w(1.0f)
    {
        NV_UNUSED(r);
    }

    /**
    \brief Constructor from a scalar: sets the real part w to the scalar value, and the imaginary parts (x,y,z) to zero
    */
    explicit NV_CUDA_CALLABLE NV_FORCE_INLINE NvQuat(float r) : x(0.0f), y(0.0f), z(0.0f), w(r)
    {
    }

    /**
    \brief Constructor.  Take note of the order of the elements!
    */
    NV_CUDA_CALLABLE NV_FORCE_INLINE NvQuat(float nx, float ny, float nz, float nw) : x(nx), y(ny), z(nz), w(nw)
    {
    }

    /**
    \brief Creates from angle-axis representation.

    Axis must be normalized!

    Angle is in radians!

    <b>Unit:</b> Radians
    */
    NV_CUDA_CALLABLE NV_INLINE NvQuat(float angleRadians, const NvVec3& unitAxis)
    {
        NV_ASSERT(NvAbs(1.0f - unitAxis.magnitude()) < 1e-3f);
        const float a = angleRadians * 0.5f;
        const float s = NvSin(a);
        w = NvCos(a);
        x = unitAxis.x * s;
        y = unitAxis.y * s;
        z = unitAxis.z * s;
    }

    /**
    \brief Copy ctor.
    */
    NV_CUDA_CALLABLE NV_FORCE_INLINE NvQuat(const NvQuat& v) : x(v.x), y(v.y), z(v.z), w(v.w)
    {
    }

    /**
    \brief Creates from orientation matrix.

    \param[in] m Rotation matrix to extract quaternion from.
    */
    NV_CUDA_CALLABLE NV_INLINE explicit NvQuat(const NvMat33& m); /* defined in NvMat33.h */

    /**
    \brief returns true if quat is identity
    */
    NV_CUDA_CALLABLE NV_FORCE_INLINE bool isIdentity() const
    {
        return x==0.0f && y==0.0f && z==0.0f && w==1.0f;
    }

    /**
    \brief returns true if all elements are finite (not NAN or INF, etc.)
    */
    NV_CUDA_CALLABLE bool isFinite() const
    {
        return NvIsFinite(x) && NvIsFinite(y) && NvIsFinite(z) && NvIsFinite(w);
    }

    /**
    \brief returns true if finite and magnitude is close to unit
    */

    NV_CUDA_CALLABLE bool isUnit() const
    {
        const float unitTolerance = 1e-4f;
        return isFinite() && NvAbs(magnitude() - 1) < unitTolerance;
    }

    /**
    \brief returns true if finite and magnitude is reasonably close to unit to allow for some accumulation of error vs
    isValid
    */

    NV_CUDA_CALLABLE bool isSane() const
    {
        const float unitTolerance = 1e-2f;
        return isFinite() && NvAbs(magnitude() - 1) < unitTolerance;
    }

    /**
    \brief returns true if the two quaternions are exactly equal
    */
    NV_CUDA_CALLABLE NV_INLINE bool operator==(const NvQuat& q) const
    {
        return x == q.x && y == q.y && z == q.z && w == q.w;
    }

    /**
    \brief converts this quaternion to angle-axis representation
    */

    NV_CUDA_CALLABLE NV_INLINE void toRadiansAndUnitAxis(float& angle, NvVec3& axis) const
    {
        const float quatEpsilon = 1.0e-8f;
        const float s2 = x * x + y * y + z * z;
        if(s2 < quatEpsilon * quatEpsilon) // can't extract a sensible axis
        {
            angle = 0.0f;
            axis = NvVec3(1.0f, 0.0f, 0.0f);
        }
        else
        {
            const float s = NvRecipSqrt(s2);
            axis = NvVec3(x, y, z) * s;
            angle = NvAbs(w) < quatEpsilon ? NvPi : NvAtan2(s2 * s, w) * 2.0f;
        }
    }

    /**
    \brief Gets the angle between this quat and the identity quaternion.

    <b>Unit:</b> Radians
    */
    NV_CUDA_CALLABLE NV_INLINE float getAngle() const
    {
        return NvAcos(w) * 2.0f;
    }

    /**
    \brief Gets the angle between this quat and the argument

    <b>Unit:</b> Radians
    */
    NV_CUDA_CALLABLE NV_INLINE float getAngle(const NvQuat& q) const
    {
        return NvAcos(dot(q)) * 2.0f;
    }

    /**
    \brief This is the squared 4D vector length, should be 1 for unit quaternions.
    */
    NV_CUDA_CALLABLE NV_FORCE_INLINE float magnitudeSquared() const
    {
        return x * x + y * y + z * z + w * w;
    }

    /**
    \brief returns the scalar product of this and other.
    */
    NV_CUDA_CALLABLE NV_FORCE_INLINE float dot(const NvQuat& v) const
    {
        return x * v.x + y * v.y + z * v.z + w * v.w;
    }

    NV_CUDA_CALLABLE NV_INLINE NvQuat getNormalized() const
    {
        const float s = 1.0f / magnitude();
        return NvQuat(x * s, y * s, z * s, w * s);
    }

    NV_CUDA_CALLABLE NV_INLINE float magnitude() const
    {
        return NvSqrt(magnitudeSquared());
    }

    // modifiers:
    /**
    \brief maps to the closest unit quaternion.
    */
    NV_CUDA_CALLABLE NV_INLINE float normalize() // convert this NvQuat to a unit quaternion
    {
        const float mag = magnitude();
        if(mag != 0.0f)
        {
            const float imag = 1.0f / mag;

            x *= imag;
            y *= imag;
            z *= imag;
            w *= imag;
        }
        return mag;
    }

    /*
    \brief returns the conjugate.

    \note for unit quaternions, this is the inverse.
    */
    NV_CUDA_CALLABLE NV_INLINE NvQuat getConjugate() const
    {
        return NvQuat(-x, -y, -z, w);
    }

    /*
    \brief returns imaginary part.
    */
    NV_CUDA_CALLABLE NV_INLINE NvVec3 getImaginaryPart() const
    {
        return NvVec3(x, y, z);
    }

    /** brief computes rotation of x-axis */
    NV_CUDA_CALLABLE NV_FORCE_INLINE NvVec3 getBasisVector0() const
    {
        const float x2 = x * 2.0f;
        const float w2 = w * 2.0f;
        return NvVec3((w * w2) - 1.0f + x * x2, (z * w2) + y * x2, (-y * w2) + z * x2);
    }

    /** brief computes rotation of y-axis */
    NV_CUDA_CALLABLE NV_FORCE_INLINE NvVec3 getBasisVector1() const
    {
        const float y2 = y * 2.0f;
        const float w2 = w * 2.0f;
        return NvVec3((-z * w2) + x * y2, (w * w2) - 1.0f + y * y2, (x * w2) + z * y2);
    }

    /** brief computes rotation of z-axis */
    NV_CUDA_CALLABLE NV_FORCE_INLINE NvVec3 getBasisVector2() const
    {
        const float z2 = z * 2.0f;
        const float w2 = w * 2.0f;
        return NvVec3((y * w2) + x * z2, (-x * w2) + y * z2, (w * w2) - 1.0f + z * z2);
    }

    /**
    rotates passed vec by this (assumed unitary)
    */
    NV_CUDA_CALLABLE NV_FORCE_INLINE const NvVec3 rotate(const NvVec3& v) const
    {
        const float vx = 2.0f * v.x;
        const float vy = 2.0f * v.y;
        const float vz = 2.0f * v.z;
        const float w2 = w * w - 0.5f;
        const float dot2 = (x * vx + y * vy + z * vz);
        return NvVec3((vx * w2 + (y * vz - z * vy) * w + x * dot2), (vy * w2 + (z * vx - x * vz) * w + y * dot2),
                      (vz * w2 + (x * vy - y * vx) * w + z * dot2));
    }

    /**
    inverse rotates passed vec by this (assumed unitary)
    */
    NV_CUDA_CALLABLE NV_FORCE_INLINE const NvVec3 rotateInv(const NvVec3& v) const
    {
        const float vx = 2.0f * v.x;
        const float vy = 2.0f * v.y;
        const float vz = 2.0f * v.z;
        const float w2 = w * w - 0.5f;
        const float dot2 = (x * vx + y * vy + z * vz);
        return NvVec3((vx * w2 - (y * vz - z * vy) * w + x * dot2), (vy * w2 - (z * vx - x * vz) * w + y * dot2),
                      (vz * w2 - (x * vy - y * vx) * w + z * dot2));
    }

    /**
    \brief Assignment operator
    */
    NV_CUDA_CALLABLE NV_FORCE_INLINE NvQuat& operator=(const NvQuat& p)
    {
        x = p.x;
        y = p.y;
        z = p.z;
        w = p.w;
        return *this;
    }

    NV_CUDA_CALLABLE NV_FORCE_INLINE NvQuat& operator*=(const NvQuat& q)
    {
        const float tx = w * q.x + q.w * x + y * q.z - q.y * z;
        const float ty = w * q.y + q.w * y + z * q.x - q.z * x;
        const float tz = w * q.z + q.w * z + x * q.y - q.x * y;

        w = w * q.w - q.x * x - y * q.y - q.z * z;
        x = tx;
        y = ty;
        z = tz;

        return *this;
    }

    NV_CUDA_CALLABLE NV_FORCE_INLINE NvQuat& operator+=(const NvQuat& q)
    {
        x += q.x;
        y += q.y;
        z += q.z;
        w += q.w;
        return *this;
    }

    NV_CUDA_CALLABLE NV_FORCE_INLINE NvQuat& operator-=(const NvQuat& q)
    {
        x -= q.x;
        y -= q.y;
        z -= q.z;
        w -= q.w;
        return *this;
    }

    NV_CUDA_CALLABLE NV_FORCE_INLINE NvQuat& operator*=(const float s)
    {
        x *= s;
        y *= s;
        z *= s;
        w *= s;
        return *this;
    }

    /** quaternion multiplication */
    NV_CUDA_CALLABLE NV_INLINE NvQuat operator*(const NvQuat& q) const
    {
        return NvQuat(w * q.x + q.w * x + y * q.z - q.y * z, w * q.y + q.w * y + z * q.x - q.z * x,
                      w * q.z + q.w * z + x * q.y - q.x * y, w * q.w - x * q.x - y * q.y - z * q.z);
    }

    /** quaternion addition */
    NV_CUDA_CALLABLE NV_FORCE_INLINE NvQuat operator+(const NvQuat& q) const
    {
        return NvQuat(x + q.x, y + q.y, z + q.z, w + q.w);
    }

    /** quaternion subtraction */
    NV_CUDA_CALLABLE NV_FORCE_INLINE NvQuat operator-() const
    {
        return NvQuat(-x, -y, -z, -w);
    }

    NV_CUDA_CALLABLE NV_FORCE_INLINE NvQuat operator-(const NvQuat& q) const
    {
        return NvQuat(x - q.x, y - q.y, z - q.z, w - q.w);
    }

    NV_CUDA_CALLABLE NV_FORCE_INLINE NvQuat operator*(float r) const
    {
        return NvQuat(x * r, y * r, z * r, w * r);
    }

    /** the quaternion elements */
    float x, y, z, w;
};

#if !NV_DOXYGEN
} // namespace nvidia
#endif

/** @} */
#endif // #ifndef NV_NVFOUNDATION_NVQUAT_H
