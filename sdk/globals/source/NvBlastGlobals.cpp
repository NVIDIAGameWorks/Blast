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


#include "NvBlastGlobals.h"
#include "NvBlastAssert.h"
#include <cstdlib>
#include <sstream>
#include <iostream>

#if NV_WINDOWS_FAMILY
#include <windows.h>
#endif

#if NV_WINDOWS_FAMILY || NV_LINUX_FAMILY
#include <malloc.h>
#endif


namespace Nv
{
namespace Blast
{

#if NV_WINDOWS_FAMILY
// on win32 we only have 8-byte alignment guaranteed, but the CRT provides special aligned allocation fns
NV_FORCE_INLINE void* platformAlignedAlloc(size_t size)
{
	return _aligned_malloc(size, 16);
}

NV_FORCE_INLINE void platformAlignedFree(void* ptr)
{
	_aligned_free(ptr);
}
#elif NV_LINUX_FAMILY
NV_FORCE_INLINE void* platformAlignedAlloc(size_t size)
{
	return ::memalign(16, size);
}

NV_FORCE_INLINE void platformAlignedFree(void* ptr)
{
	::free(ptr);
}
#elif NV_XBOXONE || NV_PS4
// on these platforms we get 16-byte alignment by default
NV_FORCE_INLINE void* platformAlignedAlloc(size_t size)
{
	return ::malloc(size);
}

NV_FORCE_INLINE void platformAlignedFree(void* ptr)
{
	::free(ptr);
}
#else
NV_FORCE_INLINE void* platformAlignedAlloc(size_t size)
{
	const int A = 16;
	unsigned char* mem = (unsigned char*)malloc(size + A);
	const unsigned char offset = (unsigned char)((uintptr_t)A - (uintptr_t)mem % A - 1);
	mem += offset;
	*mem++ = offset;
	return mem;
}

NV_FORCE_INLINE void platformAlignedFree(void* ptr)
{
	if (ptr != nullptr)
	{
		unsigned char* mem = (unsigned char*)ptr;
		const unsigned char offset = *--mem;
		::free(mem - offset);
	}
}
#endif

class DefaultAllocatorCallback : public AllocatorCallback
{
public:
	virtual void* allocate(size_t size, const char* typeName, const char* filename, int line) override
	{
		NV_UNUSED(typeName);
		NV_UNUSED(filename);
		NV_UNUSED(line);
		return platformAlignedAlloc(size);
	}

	virtual void deallocate(void* ptr) override
	{
		platformAlignedFree(ptr);
	}
};
DefaultAllocatorCallback g_defaultAllocatorCallback;


class DefaultErrorCallback : public ErrorCallback
{
	virtual void reportError(ErrorCode::Enum code, const char* msg, const char* file, int line) override
	{
#if NV_DEBUG || NV_CHECKED
		std::stringstream str;
		str << "NvBlast ";
		bool critical = false;
		switch (code)
		{
		case ErrorCode::eNO_ERROR:			str << "[Info]";				critical = false; break;
		case ErrorCode::eDEBUG_INFO:		str << "[Debug Info]";			critical = false; break;
		case ErrorCode::eDEBUG_WARNING:		str << "[Debug Warning]";		critical = false; break;
		case ErrorCode::eINVALID_PARAMETER:	str << "[Invalid Parameter]";	critical = true;  break;
		case ErrorCode::eINVALID_OPERATION:	str << "[Invalid Operation]";	critical = true;  break;
		case ErrorCode::eOUT_OF_MEMORY:		str << "[Out of] Memory";		critical = true;  break;
		case ErrorCode::eINTERNAL_ERROR:	str << "[Internal Error]";		critical = true;  break;
		case ErrorCode::eABORT:				str << "[Abort]";				critical = true;  break;
		case ErrorCode::ePERF_WARNING:		str << "[Perf Warning]";		critical = false; break;
		default:							NVBLAST_ASSERT(false);
		}
		str << file << "(" << line << "): " << msg << "\n";

		std::string message = str.str();
		std::cout << message;
#if NV_WINDOWS_FAMILY
		OutputDebugStringA(message.c_str());
#endif 
		NVBLAST_ASSERT_WITH_MESSAGE(!critical, message.c_str());
#else
		NV_UNUSED(code);
		NV_UNUSED(msg);
		NV_UNUSED(file);
		NV_UNUSED(line);
#endif
	}
};
DefaultErrorCallback g_defaultErrorCallback;


AllocatorCallback* g_allocatorCallback = &g_defaultAllocatorCallback;
ErrorCallback* g_errorCallback = &g_defaultErrorCallback;


} // namespace Blast
} // namespace Nv


//////// Global API implementation ////////

Nv::Blast::AllocatorCallback* NvBlastGlobalGetAllocatorCallback()
{
	return Nv::Blast::g_allocatorCallback;
}

void NvBlastGlobalSetAllocatorCallback(Nv::Blast::AllocatorCallback* allocator)
{
	Nv::Blast::g_allocatorCallback = allocator ? allocator : &Nv::Blast::g_defaultAllocatorCallback;
}

Nv::Blast::ErrorCallback* NvBlastGlobalGetErrorCallback()
{
	return Nv::Blast::g_errorCallback;
}

void NvBlastGlobalSetErrorCallback(Nv::Blast::ErrorCallback* errorCallback)
{
	Nv::Blast::g_errorCallback = errorCallback ? errorCallback : &Nv::Blast::g_defaultErrorCallback;
}
