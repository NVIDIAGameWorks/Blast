/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTPROFILER_H
#define NVBLASTPROFILER_H

#include "NvBlastPreprocessor.h"

namespace physx {
	class PxProfilerCallback;
}

struct NvBlastProfilerDetail
{
	enum Level
	{
		LOW,
		MEDIUM,
		HIGH
	};
};

/**
Profiler features are only active in checked, debug and profile builds.
*/

/**
Set a callback to PVD or another PxProfilerCallback based profiler.
*/
NVBLAST_API void NvBlastProfilerSetCallback(physx::PxProfilerCallback* pcb);

/**
Enable events for platform specific profiler tools. Currently supported:
Nsight, PS4, Xbox One
*/
NVBLAST_API void NvBlastProfilerEnablePlatform(bool);

/**
Sets the depth of reported profile zones.
Higher levels (more nesting) of instrumentation can have a significant impact.
Defaults to NvBlastProfilerDetail::Level::LOW.
*/
NVBLAST_API void NvBlastProfilerSetDetail(NvBlastProfilerDetail::Level);

#endif
