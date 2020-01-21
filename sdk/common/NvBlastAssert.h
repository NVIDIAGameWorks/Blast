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


#ifndef NVBLASTASSERT_H
#define NVBLASTASSERT_H


#include "NvBlastPreprocessor.h"


#if !NV_ENABLE_ASSERTS
#define NVBLAST_ASSERT(exp) ((void)0)
#define NVBLAST_ALWAYS_ASSERT_MESSAGE(message) ((void)0)
#define NVBLAST_ASSERT_WITH_MESSAGE(condition, message) ((void)0)
#else
#if NV_VC
#define NVBLAST_CODE_ANALYSIS_ASSUME(exp)                                                                     \
	__analysis_assume(!!(exp)) // This macro will be used to get rid of analysis warning messages if a NVBLAST_ASSERT is used
// to "guard" illegal mem access, for example.
#else
#define NVBLAST_CODE_ANALYSIS_ASSUME(exp)
#endif
#define NVBLAST_ASSERT(exp)                                                                                   \
{                                                                                                             \
	static bool _ignore = false;                                                                              \
	if (!(exp) && !_ignore) NvBlastAssertHandler(#exp, __FILE__, __LINE__, _ignore);                          \
	NVBLAST_CODE_ANALYSIS_ASSUME(exp);                                                                        \
} ((void)0)
#define NVBLAST_ALWAYS_ASSERT_MESSAGE(message)                                                                    \
{                                                                                                             \
	static bool _ignore = false;                                                                              \
	if(!_ignore)                                                                                              \
	{                                                                                                         \
		NvBlastAssertHandler(message, __FILE__, __LINE__, _ignore);                                               \
	}                                                                                                         \
} ((void)0)
#define NVBLAST_ASSERT_WITH_MESSAGE(exp, message)                                                             \
{                                                                                                             \
	static bool _ignore = false;                                                                              \
	if (!(exp) && !_ignore) NvBlastAssertHandler(message, __FILE__, __LINE__, _ignore);                       \
	NVBLAST_CODE_ANALYSIS_ASSUME(exp);                                                                        \
} ((void)0)
#endif

#define NVBLAST_ALWAYS_ASSERT() NVBLAST_ASSERT(0)


extern "C"
{

NVBLAST_API void NvBlastAssertHandler(const char* expr, const char* file, int line, bool& ignore);

} // extern "C"


#endif // #ifndef NVBLASTASSERT_H
