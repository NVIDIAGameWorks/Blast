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

#ifndef NV_NVFOUNDATION_NVTRANSFORM_H
#define NV_NVFOUNDATION_NVTRANSFORM_H
/** \addtogroup foundation
  @{
*/

#include "NvQuat.h"
#include "NvPlane.h"

#if !NV_DOXYGEN
namespace nvidia
{
#endif

/*!
\brief class representing a rigid euclidean transform as a quaternion and a vector
*/

class NvTransform
{
  public:
    NvQuat q;
    NvVec3 p;

    NV_CUDA_CALLABLE NV_FORCE_INLINE NvTransform()
    {
    }

    NV_CUDA_CALLABLE NV_FORCE_INLINE explicit NvTransform(const NvVec3& position) : q(NvIdentity), p(position)
    {
    }

    NV_CUDA_CALLABLE NV_FORCE_INLINE explicit NvTransform(NvIDENTITY r) : q(NvIdentity), p(NvZero)
    {
        NV_UNUSED(r);
    }

    NV_CUDA_CALLABLE NV_FORCE_INLINE explicit NvTransform(const NvQuat& orientation) : q(orientation), p(0)
    {
        NV_ASSERT(orientation.isSane());
    }

    NV_CUDA_CALLABLE NV_FORCE_INLINE NvTransform(float x, float y, float z, NvQuat aQ = NvQuat(NvIdentity))
    : q(aQ), p(x, y, z)
    {
    }

    NV_CUDA_CALLABLE NV_FORCE_INLINE NvTransform(const NvVec3& p0, const NvQuat& q0) : q(q0), p(p0)
    {
        NV_ASSERT(q0.isSane());
    }

    NV_CUDA_CALLABLE NV_FORCE_INLINE explicit NvTransform(const NvMat44& m); // defined in NvMat44.h

    /**
    \brief returns true if the two transforms are exactly equal
    */
    NV_CUDA_CALLABLE NV_INLINE bool operator==(const NvTransform& t) const
    {
        return p == t.p && q == t.q;
    }

    NV_CUDA_CALLABLE NV_FORCE_INLINE NvTransform operator*(const NvTransform& x) const
    {
        NV_ASSERT(x.isSane());
        return transform(x);
    }

    //! Equals matrix multiplication
    NV_CUDA_CALLABLE NV_INLINE NvTransform& operator*=(NvTransform& other)
    {
        *this = *this * other;
        return *this;
    }

    NV_CUDA_CALLABLE NV_FORCE_INLINE NvTransform getInverse() const
    {
        NV_ASSERT(isFinite());
        return NvTransform(q.rotateInv(-p), q.getConjugate());
    }

    NV_CUDA_CALLABLE NV_FORCE_INLINE NvVec3 transform(const NvVec3& input) const
    {
        NV_ASSERT(isFinite());
        return q.rotate(input) + p;
    }

    NV_CUDA_CALLABLE NV_FORCE_INLINE NvVec3 transformInv(const NvVec3& input) const
    {
        NV_ASSERT(isFinite());
        return q.rotateInv(input - p);
    }

    NV_CUDA_CALLABLE NV_FORCE_INLINE NvVec3 rotate(const NvVec3& input) const
    {
        NV_ASSERT(isFinite());
        return q.rotate(input);
    }

    NV_CUDA_CALLABLE NV_FORCE_INLINE NvVec3 rotateInv(const NvVec3& input) const
    {
        NV_ASSERT(isFinite());
        return q.rotateInv(input);
    }

    //! Transform transform to parent (returns compound transform: first src, then *this)
    NV_CUDA_CALLABLE NV_FORCE_INLINE NvTransform transform(const NvTransform& src) const
    {
        NV_ASSERT(src.isSane());
        NV_ASSERT(isSane());
        // src = [srct, srcr] -> [r*srct + t, r*srcr]
        return NvTransform(q.rotate(src.p) + p, q * src.q);
    }

    /**
    \brief returns true if finite and q is a unit quaternion
    */

    NV_CUDA_CALLABLE bool isValid() const
    {
        return p.isFinite() && q.isFinite() && q.isUnit();
    }

    /**
    \brief returns true if finite and quat magnitude is reasonably close to unit to allow for some accumulation of error
    vs isValid
    */

    NV_CUDA_CALLABLE bool isSane() const
    {
        return isFinite() && q.isSane();
    }

    /**
    \brief returns true if all elems are finite (not NAN or INF, etc.)
    */
    NV_CUDA_CALLABLE NV_FORCE_INLINE bool isFinite() const
    {
        return p.isFinite() && q.isFinite();
    }

    //! Transform transform from parent (returns compound transform: first src, then this->inverse)
    NV_CUDA_CALLABLE NV_FORCE_INLINE NvTransform transformInv(const NvTransform& src) const
    {
        NV_ASSERT(src.isSane());
        NV_ASSERT(isFinite());
        // src = [srct, srcr] -> [r^-1*(srct-t), r^-1*srcr]
        NvQuat qinv = q.getConjugate();
        return NvTransform(qinv.rotate(src.p - p), qinv * src.q);
    }

    /**
    \brief transform plane
    */

    NV_CUDA_CALLABLE NV_FORCE_INLINE NvPlane transform(const NvPlane& plane) const
    {
        NvVec3 transformedNormal = rotate(plane.n);
        return NvPlane(transformedNormal, plane.d - p.dot(transformedNormal));
    }

    /**
    \brief inverse-transform plane
    */

    NV_CUDA_CALLABLE NV_FORCE_INLINE NvPlane inverseTransform(const NvPlane& plane) const
    {
        NvVec3 transformedNormal = rotateInv(plane.n);
        return NvPlane(transformedNormal, plane.d + p.dot(plane.n));
    }

    /**
    \brief return a normalized transform (i.e. one in which the quaternion has unit magnitude)
    */
    NV_CUDA_CALLABLE NV_FORCE_INLINE NvTransform getNormalized() const
    {
        return NvTransform(p, q.getNormalized());
    }
};

#if !NV_DOXYGEN
} // namespace nvidia
#endif

/** @} */
#endif // #ifndef NV_NVFOUNDATION_NVTRANSFORM_H
