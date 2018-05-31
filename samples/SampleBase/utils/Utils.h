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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.


#ifndef UTILS_H
#define UTILS_H

#include <DeviceManager.h>
#include <assert.h>

#include "PxPreprocessor.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//														MACROS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef V_RETURN
#define V_RETURN(x)                                                                                                    \
	{                                                                                                                  \
		hr = (x);                                                                                                      \
		if(FAILED(hr))                                                                                                 \
		{                                                                                                              \
			return hr;                                                                                                 \
		}                                                                                                              \
	}
#endif

#ifndef V
#define V(x)                                                                                                           \
	{                                                                                                                  \
		HRESULT hr = (x);                                                                                              \
		_ASSERT(SUCCEEDED(hr));                                                                                        \
	}
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)                                                                                                \
	{                                                                                                                  \
		if(p)                                                                                                          \
		{                                                                                                              \
			(p)->Release();                                                                                            \
			(p) = NULL;                                                                                                \
		}                                                                                                              \
	}
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)                                                                                                 \
	{                                                                                                                  \
		if(p)                                                                                                          \
		{                                                                                                              \
			delete (p);                                                                                                \
			(p) = NULL;                                                                                                \
		}                                                                                                              \
	}
#endif

#define ASSERT_PRINT(cond, format, ...)                                                                                \
	if(!(cond))                                                                                                        \
	{                                                                                                                  \
		messagebox_printf("Assertion Failed!", MB_OK | MB_ICONERROR, #cond "\n" format, __VA_ARGS__);                  \
		assert(cond);                                                                                                  \
	}

HRESULT messagebox_printf(const char* caption, UINT mb_type, const char* format, ...);



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static const char* strext(const char* str)
{
	const char* ext = NULL; // by default no extension found!
	while (str)
	{
		str = strchr(str, '.');
		if (str)
		{
			str++;
			ext = str;
		}
	}
	return ext;
}

static inline float  lerp(float a, float b, float t)                          { return a + (b - a) * t; }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif