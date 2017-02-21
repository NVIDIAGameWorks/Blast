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