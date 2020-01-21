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


#ifndef NVBLASTGLOBALS_H
#define NVBLASTGLOBALS_H

#include <new>
#include "NvBlastTypes.h"


namespace Nv
{
namespace Blast
{


/**
\brief Abstract base class for an application defined memory allocator that can be used by toolkit (Tk) or any extension (Ext).
*/
class AllocatorCallback
{
public:
	/**
	\brief destructor
	*/
	virtual ~AllocatorCallback()
	{
	}

	/**
	\brief Allocates size bytes of memory, which must be 16-byte aligned.

	This method should never return NULL.  If you run out of memory, then
	you should terminate the app or take some other appropriate action.

	\param size			Number of bytes to allocate.
	\param typeName		Name of the datatype that is being allocated
	\param filename		The source file which allocated the memory
	\param line			The source line which allocated the memory
	\return				The allocated block of memory.
	*/
	virtual void* allocate(size_t size, const char* typeName, const char* filename, int line) = 0;

	/**
	\brief Frees memory previously allocated by allocate().

	\param ptr Memory to free.
	*/
	virtual void deallocate(void* ptr) = 0;
};


/**
\brief Error codes

These error codes are passed to #ErrorCallback

\note: It's actually the copy of PxErrorCallback's PxErrorCode so it can be easily casted to it. Keep that 
in mind if you are going to change this enum.

@see ErrorCallback
*/
struct ErrorCode
{
	enum Enum
	{
		eNO_ERROR = 0,

		//! \brief An informational message.
		eDEBUG_INFO = 1,

		//! \brief a warning message for the user to help with debugging
		eDEBUG_WARNING = 2,

		//! \brief method called with invalid parameter(s)
		eINVALID_PARAMETER = 4,

		//! \brief method was called at a time when an operation is not possible
		eINVALID_OPERATION = 8,

		//! \brief method failed to allocate some memory
		eOUT_OF_MEMORY = 16,

		/** \brief The library failed for some reason.
		Possibly you have passed invalid values like NaNs, which are not checked for.
		*/
		eINTERNAL_ERROR = 32,

		//! \brief An unrecoverable error, execution should be halted and log output flushed
		eABORT = 64,

		//! \brief The SDK has determined that an operation may result in poor performance.
		ePERF_WARNING = 128,

		//! \brief A bit mask for including all errors
		eMASK_ALL = -1
	};
};


/**
\brief User defined interface class.  Used by the library to emit debug information.

\note The SDK state should not be modified from within any error reporting functions.
*/
class ErrorCallback
{
public:
	virtual ~ErrorCallback()
	{
	}

	/**
	\brief Reports an error code.
	\param code Error code, see #ErrorCode
	\param message Message to display.
	\param file File error occured in.
	\param line Line number error occured on.
	*/
	virtual void reportError(ErrorCode::Enum code, const char* message, const char* file, int line) = 0;
};


} // namespace Blast
} // namespace Nv


//////// Global API to Access Global AllocatorCallback and ErrorCallback ////////

/**
Retrieve a pointer to the global AllocatorCallback. Default implementation with std allocator is used if user didn't provide
it's own. It always exist, 'nullptr' will never be returned.

\return the pointer to the global AllocatorCallback.
*/
NVBLAST_API Nv::Blast::AllocatorCallback* NvBlastGlobalGetAllocatorCallback();

/**
Set global AllocatorCallback.  If 'nullptr' is passed the default AllocatorCallback with std allocator is set.
*/
NVBLAST_API void NvBlastGlobalSetAllocatorCallback(Nv::Blast::AllocatorCallback* allocatorCallback);

/**
Retrieve a pointer to the global ErrorCallback. Default implementation which writes messages to stdout is used if user didn't provide
it's own. It always exist, 'nullptr' will never be returned.

\return the pointer to the global ErrorCallback.
*/
NVBLAST_API Nv::Blast::ErrorCallback* NvBlastGlobalGetErrorCallback();

/**
Set global ErrorCallback.  If 'nullptr' is passed the default ErrorCallback that writes messages to stdout is set.
*/
NVBLAST_API void NvBlastGlobalSetErrorCallback(Nv::Blast::ErrorCallback* errorCallback);


//////// Helper Global Functions ////////

namespace Nv
{
namespace Blast
{


/**
Logging wrapper compatible with NvBlastLog. @see NvBlastLog.

Pass this function to LowLevel function calls in order to get logging into global ErrorCallback.
*/
NV_INLINE void logLL(int type, const char* msg, const char* file, int line)
{
	ErrorCode::Enum errorCode = ErrorCode::eNO_ERROR;
	switch (type)
	{
	case NvBlastMessage::Error:		errorCode = ErrorCode::eINVALID_OPERATION;	break;
	case NvBlastMessage::Warning:	errorCode = ErrorCode::eDEBUG_WARNING;		break;
	case NvBlastMessage::Info:		errorCode = ErrorCode::eDEBUG_INFO;			break;
	case NvBlastMessage::Debug:		errorCode = ErrorCode::eNO_ERROR;			break;
	}

	NvBlastGlobalGetErrorCallback()->reportError(errorCode, msg, file, line);
}


} // namespace Blast
} // namespace Nv



//////// Allocator macros ////////

/**
Alloc/Free macros that use global AllocatorCallback.  Thus allocated memory is 16-byte aligned.
*/
#define NVBLAST_ALLOC(_size)				NvBlastGlobalGetAllocatorCallback()->allocate(_size, nullptr, __FILE__, __LINE__)
#define NVBLAST_ALLOC_NAMED(_size, _name)	NvBlastGlobalGetAllocatorCallback()->allocate(_size, _name, __FILE__, __LINE__)
#define NVBLAST_FREE(_mem)					NvBlastGlobalGetAllocatorCallback()->deallocate(_mem)

/**
Placement new with ExtContext allocation.
Example: Foo* foo = NVBLAST_NEW(Foo, context) (params);
*/
#define NVBLAST_NEW(T) new (NvBlastGlobalGetAllocatorCallback()->allocate(sizeof(T), #T, __FILE__, __LINE__)) T

/**
Respective delete to NVBLAST_NEW
Example: NVBLAST_DELETE(foo, Foo, context);
*/
#define NVBLAST_DELETE(obj, T)					\
	(obj)->~T();								\
	NvBlastGlobalGetAllocatorCallback()->deallocate(obj)



//////// Log macros ////////

/**
Logging macros that use global AllocatorCallback.
*/
#define NVBLAST_LOG(_code, _msg)	NvBlastGlobalGetErrorCallback()->reportError(_code, _msg, __FILE__, __LINE__)
#define NVBLAST_LOG_ERROR(_msg)		NVBLAST_LOG(Nv::Blast::ErrorCode::eINVALID_OPERATION, _msg)
#define NVBLAST_LOG_WARNING(_msg)	NVBLAST_LOG(Nv::Blast::ErrorCode::eDEBUG_WARNING, _msg)
#define NVBLAST_LOG_INFO(_msg)		NVBLAST_LOG(Nv::Blast::ErrorCode::eDEBUG_INFO, _msg)
#define NVBLAST_LOG_DEBUG(_msg)		NVBLAST_LOG(Nv::Blast::ErrorCode::eNO_ERROR, _msg)

/**
Check macros that use global AllocatorCallback. The idea is that you pass an expression to check, if it fails
it logs and calls '_onFail' code you passed.
*/
#define NVBLAST_CHECK(_code, _expr, _msg, _onFail)																		\
	{																													\
		if(!(_expr))																									\
		{																												\
			NVBLAST_LOG(_code, _msg);																					\
			{ _onFail; };																								\
		}																												\
	}																													

#define NVBLAST_CHECK_ERROR(_expr, _msg, _onFail)	NVBLAST_CHECK(Nv::Blast::ErrorCode::eINVALID_OPERATION, _expr, _msg, _onFail)
#define NVBLAST_CHECK_WARNING(_expr, _msg, _onFail)	NVBLAST_CHECK(Nv::Blast::ErrorCode::eDEBUG_WARNING, _expr, _msg, _onFail)
#define NVBLAST_CHECK_INFO(_expr, _msg, _onFail)	NVBLAST_CHECK(Nv::Blast::ErrorCode::eDEBUG_INFO, _expr, _msg, _onFail)
#define NVBLAST_CHECK_DEBUG(_expr, _msg, _onFail)	NVBLAST_CHECK(Nv::Blast::ErrorCode::eNO_ERROR, _expr, _msg, _onFail)


//////// Misc ////////


// Macro to load a uint32_t (or larger) with four characters
#define NVBLAST_FOURCC(_a, _b, _c, _d)	( (uint32_t)(_a) | (uint32_t)(_b)<<8 | (uint32_t)(_c)<<16 | (uint32_t)(_d)<<24 )


#endif // ifndef NVBLASTGLOBALS_H
