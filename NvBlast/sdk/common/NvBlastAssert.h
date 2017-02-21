/*
 * Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef NVBLASTASSERT_H
#define NVBLASTASSERT_H


#include "NvBlastPreprocessor.h"


#if !NV_ENABLE_ASSERTS
#define NVBLAST_ASSERT(exp) ((void)0)
#define NVBLAST_ALWAYS_ASSERT_MESSAGE(exp) ((void)0)
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
#define NVBLAST_ALWAYS_ASSERT_MESSAGE(exp)                                                                    \
{                                                                                                             \
	static bool _ignore = false;                                                                              \
	if(!_ignore)                                                                                              \
	{                                                                                                         \
		NvBlastAssertHandler(exp, __FILE__, __LINE__, _ignore);                                               \
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
