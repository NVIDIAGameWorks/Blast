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

#ifndef NV_NSFOUNDATION_NSFPU_H
#define NV_NSFOUNDATION_NSFPU_H

#include "Ns.h"
#include "NsIntrinsics.h"

// unsigned integer representation of a floating-point value.
#if NV_PS3

NV_FORCE_INLINE unsigned int NV_IR(const float x)
{
    union
    {
        int i;
        float f;
    } u;
    u.f = x;
    return u.i;
}

NV_FORCE_INLINE int NV_SIR(const float x)
{
    union
    {
        int i;
        float f;
    } u;

    u.f = x;
    return u.i;
}

NV_FORCE_INLINE float NV_FR(const unsigned int x)
{
    union
    {
        unsigned int i;
        float f;
    } u;
    u.i = x;
    return u.f;
}

#else
#define NV_IR(x) ((uint32_t&)(x))
#define NV_SIR(x) ((int32_t&)(x))
#define NV_FR(x) ((float&)(x))
#endif

// signed integer representation of a floating-point value.

// Floating-point representation of a integer value.

#define NV_SIGN_BITMASK 0x80000000

#define NV_FPU_GUARD shdfnd::FPUGuard scopedFpGuard;
#define NV_SIMD_GUARD shdfnd::SIMDGuard scopedFpGuard;

#define NV_SUPPORT_GUARDS (NV_WINDOWS_FAMILY || NV_XBOXONE || NV_LINUX || NV_PS4 || NV_OSX)

namespace nvidia
{
namespace shdfnd
{
// sets the default SDK state for scalar and SIMD units
class NV_FOUNDATION_API FPUGuard
{
  public:
    FPUGuard();  // set fpu control word for PhysX
    ~FPUGuard(); // restore fpu control word
  private:
    uint32_t mControlWords[8];
};

// sets default SDK state for simd unit only, lighter weight than FPUGuard
class SIMDGuard
{
  public:
    NV_INLINE SIMDGuard();  // set simd control word for PhysX
    NV_INLINE ~SIMDGuard(); // restore simd control word
  private:
#if NV_SUPPORT_GUARDS
    uint32_t mControlWord;
#endif
};

/**
\brief Enables floating point exceptions for the scalar and SIMD unit
*/
NV_FOUNDATION_API void enableFPExceptions();

/**
\brief Disables floating point exceptions for the scalar and SIMD unit
*/
NV_FOUNDATION_API void disableFPExceptions();

} // namespace shdfnd
} // namespace nvidia

#if NV_WINDOWS_FAMILY || NV_XBOXONE
#include "platform/windows/NsWindowsFPU.h"
#elif NV_LINUX || NV_PS4 || NV_OSX
#include "platform/unix/NsUnixFPU.h"
#else
NV_INLINE nvidia::shdfnd::SIMDGuard::SIMDGuard()
{
}
NV_INLINE nvidia::shdfnd::SIMDGuard::~SIMDGuard()
{
}
#endif

#endif // #ifndef NV_NSFOUNDATION_NSFPU_H
