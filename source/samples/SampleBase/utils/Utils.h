// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2008-2022 NVIDIA Corporation. All rights reserved.


#ifndef UTILS_H
#define UTILS_H

#include <DeviceManager.h>
#include <assert.h>

#include "PxPreprocessor.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                      MACROS
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