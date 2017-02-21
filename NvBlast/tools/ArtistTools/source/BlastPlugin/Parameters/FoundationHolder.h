// This code contains NVIDIA Confidential Information and is disclosed 
// under the Mutual Non-Disclosure Agreement. 
// 
// Notice 
// ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES 
// NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO 
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT, 
// MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. 
// 
// NVIDIA Corporation assumes no responsibility for the consequences of use of such 
// information or for any infringement of patents or other rights of third parties that may 
// result from its use. No license is granted by implication or otherwise under any patent 
// or patent rights of NVIDIA Corporation. No third party distribution is allowed unless 
// expressly authorized by NVIDIA.  Details are subject to change without notice. 
// This code supersedes and replaces all information previously supplied. 
// NVIDIA Corporation products are not authorized for use as critical 
// components in life support devices or systems without express written approval of 
// NVIDIA Corporation. 
// 
// Copyright (c) 2013 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//

#pragma once

#include "NvErrorCallback.h"
#include "NsGlobals.h"
#include "NsVersionNumber.h"

class DefaultErrorCallback : public nvidia::NvErrorCallback
{
public:
	DefaultErrorCallback(void)
	{
	}

	virtual void reportError(nvidia::NvErrorCode::Enum code, const char* message, const char* file, int line)
	{
		NV_UNUSED(code);
		printf("PhysX: %s : %s : %d\r\n", message, file, line);
	}
private:
};

class DefaultAllocator : public nvidia::NvAllocatorCallback
{
public:
	DefaultAllocator(void)
	{
	}

	~DefaultAllocator(void)
	{
	}

	virtual void* allocate(size_t size, const char* typeName, const char* filename, int line)
	{
		NV_UNUSED(typeName);
		NV_UNUSED(filename);
		NV_UNUSED(line);
		void *ret = ::_aligned_malloc(size, 16);
		return ret;
	}

	virtual void deallocate(void* ptr)
	{
		::_aligned_free(ptr);
	}
private:
};


#if 0
class FoundationHolder
{
	NvFoundation* mFoundation;
	FoundationHolder()
		:mFoundation(nullptr)
	{
	}

	~FoundationHolder()
	{
		if (mFoundation)
		{
			// to-do
			// we should release foundation. but Hair SDK could release it first.
			//mFoundation->release();
			mFoundation = nullptr;
		}
	}

public:

	static NvFoundation* GetFoundation()
	{
		static FoundationHolder fh;
		if (fh.mFoundation == nullptr)
		{
			static DefaultAllocator sDefaultAllocator;
			static DefaultErrorCallback sDefaultErrorCallback;
			fh.mFoundation = NvCreateFoundation(NV_FOUNDATION_VERSION, sDefaultAllocator, sDefaultErrorCallback);
			assert(fh.mFoundation != nullptr);
		}
		return fh.mFoundation;
	}
};

#else

class FoundationHolder
{
	bool m_isInitialized;
	public:
	static void GetFoundation()
	{
		static FoundationHolder s_holder;
		if (!s_holder.m_isInitialized)
		{
			static DefaultAllocator sDefaultAllocator;
			static DefaultErrorCallback sDefaultErrorCallback;
			nvidia::shdfnd::initializeSharedFoundation(NV_FOUNDATION_VERSION, sDefaultAllocator, sDefaultErrorCallback);

			s_holder.m_isInitialized = true;
		}
	}
	~FoundationHolder()
	{
		//nvidia::terminateSharedFoundation();
	}
};

#endif
