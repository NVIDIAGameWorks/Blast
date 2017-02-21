/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTPREPROCESSORINTERNAL_H
#define NVBLASTPREPROCESSORINTERNAL_H


#include "NvPreprocessor.h"


/** Blast will check function parameters for debug and checked builds. */
#define NVBLAST_CHECK_PARAMS (NV_DEBUG || NV_CHECKED)


#if NVBLAST_CHECK_PARAMS
#define NVBLAST_CHECK(_expr, _logFn, _msg, _onFail)																		\
	{																													\
		if(!(_expr))																									\
		{																												\
			if (_logFn) { _logFn(NvBlastMessage::Error, _msg, __FILE__, __LINE__); }									\
			{ _onFail; };																								\
		}																												\
	}																													
#else
#define NVBLAST_CHECK(_expr, _logFn, _msg, _onFail) NV_UNUSED(_logFn)
#endif


#endif // ifndef NVBLASTPREPROCESSORINTERNAL_H
