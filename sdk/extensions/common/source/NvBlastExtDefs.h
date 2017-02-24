/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTEXTDEFS_H
#define NVBLASTEXTDEFS_H

#include "NvBlastTkFramework.h"
#include "PxAllocatorCallback.h"
#include <new>


//////// Log macros that use the ExtContext log function ////////

#define NVBLASTEXT_LOG_ERROR(_msg)		NVBLAST_LOG_ERROR(NvBlastTkFrameworkGet()->getLogFn(), _msg)
#define NVBLASTEXT_LOG_WARNING(_msg)	NVBLAST_LOG_WARNING(NvBlastTkFrameworkGet()->getLogFn(), _msg)
#define NVBLASTEXT_LOG_INFO(_msg)		NVBLAST_LOG_INFO(NvBlastTkFrameworkGet()->getLogFn(), _msg)
#define NVBLASTEXT_LOG_DEBUG(_msg)		NVBLAST_LOG_DEBUG(NvBlastTkFrameworkGet()->getLogFn(), _msg)

#define NVBLASTEXT_CHECK(_expr, _messageType, _msg, _onFail)								\
	{																						\
		if(!(_expr))																		\
		{																					\
			(*NvBlastTkFrameworkGet()->getLogFn())(_messageType, _msg, __FILE__, __LINE__);	\
			{ _onFail; };																	\
		}																					\
	}																													

#define NVBLASTEXT_CHECK_ERROR(_expr, _msg, _onFail)	NVBLASTEXT_CHECK(_expr, NvBlastMessage::Error, _msg, _onFail)
#define NVBLASTEXT_CHECK_WARNING(_expr, _msg, _onFail)	NVBLASTEXT_CHECK(_expr, NvBlastMessage::Warning, _msg, _onFail)
#define NVBLASTEXT_CHECK_INFO(_expr, _msg, _onFail)		NVBLASTEXT_CHECK(_expr, NvBlastMessage::Info, _msg, _onFail)
#define NVBLASTEXT_CHECK_DEBUG(_expr, _msg, _onFail)	NVBLASTEXT_CHECK(_expr, NvBlastMessage::Debug, _msg, _onFail)


//////// Allocator macros ////////

/**
Placement new with ExtContext allocation.
Example: Foo* foo = NVBLASTEXT_NEW(Foo, context) (params);
*/
#define NVBLASTEXT_NEW(T) new (NvBlastTkFrameworkGet()->getAllocatorCallback().allocate(sizeof(T), #T, __FILE__, __LINE__)) T

/**
Respective delete to NVBLASTEXT_NEW
Example: NVBLASTEXT_DELETE(foo, Foo, context);
*/
#define NVBLASTEXT_DELETE(obj, T)				\
	(obj)->~T();								\
	NvBlastTkFrameworkGet()->getAllocatorCallback().deallocate(obj)


//////// Util macros ////////

// Macro to load a uint32_t (or larger) with four characters
#define NVBLASTEXT_FOURCC(_a, _b, _c, _d)	( (uint32_t)(_a) | (uint32_t)(_b)<<8 | (uint32_t)(_c)<<16 | (uint32_t)(_d)<<24 )


#endif // #ifndef NVBLASTEXTDEFS_H
