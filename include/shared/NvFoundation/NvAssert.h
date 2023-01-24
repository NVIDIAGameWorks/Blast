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

#ifndef NV_NVFOUNDATION_NVASSERT_H
#define NV_NVFOUNDATION_NVASSERT_H

/** \addtogroup foundation
@{ */

#include "Nv.h"

#if !NV_DOXYGEN
namespace nvidia
{
#endif

/* Base class to handle assert failures */
class NvAssertHandler
{
  public:
    virtual ~NvAssertHandler()
    {
    }
    virtual void operator()(const char* exp, const char* file, int line, bool& ignore) = 0;
};

NV_FOUNDATION_API NvAssertHandler& NvGetAssertHandler();
NV_FOUNDATION_API void NvSetAssertHandler(NvAssertHandler& handler);

#if !NV_DOXYGEN
} // namespace nvidia
#endif

#if !NV_ENABLE_ASSERTS
#define NV_ASSERT(exp) ((void)0)
#define NV_ALWAYS_ASSERT_MESSAGE(exp) ((void)0)
#define NV_ASSERT_WITH_MESSAGE(condition, message) ((void)0)
#elif NV_SPU
#include "ps3/NvPS3Assert.h"
#else
#if NV_VC
#define NV_CODE_ANALYSIS_ASSUME(exp)                                                                                   \
    __analysis_assume(!!(exp)) // This macro will be used to get rid of analysis warning messages if a NV_ASSERT is used
// to "guard" illegal mem access, for example.
#else
#define NV_CODE_ANALYSIS_ASSUME(exp)
#endif
#define NV_ASSERT(exp)                                                                                                 \
    {                                                                                                                  \
        static bool _ignore = false;                                                                                   \
        ((void)((!!(exp)) || (!_ignore && (nvidia::NvGetAssertHandler()(#exp, __FILE__, __LINE__, _ignore), false)))); \
        NV_CODE_ANALYSIS_ASSUME(exp);                                                                                  \
    }
#define NV_ALWAYS_ASSERT_MESSAGE(exp)                                                                                  \
    {                                                                                                                  \
        static bool _ignore = false;                                                                                   \
        if(!_ignore)                                                                                                   \
            nvidia::NvGetAssertHandler()(exp, __FILE__, __LINE__, _ignore);                                            \
    }
#define NV_ASSERT_WITH_MESSAGE(exp, message)                                                                              \
    {                                                                                                                     \
        static bool _ignore = false;                                                                                      \
        ((void)((!!(exp)) || (!_ignore && (nvidia::NvGetAssertHandler()(message, __FILE__, __LINE__, _ignore), false)))); \
        NV_CODE_ANALYSIS_ASSUME(exp);                                                                                     \
    }
#endif

#define NV_ALWAYS_ASSERT() NV_ASSERT(0)

/** @} */
#endif // #ifndef NV_NVFOUNDATION_NVASSERT_H
