/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTPREPROCESSOR_H
#define NVBLASTPREPROCESSOR_H


#include "NvPreprocessor.h"


/** Blast API declaration */
#define NVBLAST_API NV_C_EXPORT NV_DLL_EXPORT


/**
Macros for more convenient logging
*/
#define NVBLAST_LOG_ERROR(_logFn, _msg)		if (_logFn != nullptr) { _logFn(NvBlastMessage::Error, _msg, __FILE__, __LINE__); } ((void)0)
#define NVBLAST_LOG_WARNING(_logFn, _msg)	if (_logFn != nullptr) { _logFn(NvBlastMessage::Warning, _msg, __FILE__, __LINE__); } ((void)0)
#define NVBLAST_LOG_INFO(_logFn, _msg)		if (_logFn != nullptr) { _logFn(NvBlastMessage::Info, _msg, __FILE__, __LINE__); } ((void)0)
#define NVBLAST_LOG_DEBUG(_logFn, _msg)		if (_logFn != nullptr) { _logFn(NvBlastMessage::Debug, _msg, __FILE__, __LINE__); } ((void)0)


#endif // ifndef NVBLASTPREPROCESSOR_H
