/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NvBlastProfilerInternal.h"
#include "PxProfiler.h"

#if NV_PROFILE || NV_CHECKED || NV_DEBUG

#if NV_NVTX  
#include "nvToolsExt.h"
NV_INLINE void platformZoneStart(const char* name) { nvtxRangePush(name); }
NV_INLINE void platformZoneEnd(const char*)		 { nvtxRangePop(); }

#elif NV_XBOXONE
#include "xboxone/NvBlastProfilerXB1.h"

#elif NV_PS4
#include "ps4/NvBlastProfilerPS4.h"

#else
NV_INLINE void platformZoneStart(const char*)	{ }
NV_INLINE void platformZoneEnd(const char*)	{ }

#endif

static const uint64_t blastContextId = 0xb1a57;
static physx::PxProfilerCallback* sCallback = nullptr;
static bool sPlatform = false;
static NvBlastProfilerDetail::Level sDetail = NvBlastProfilerDetail::LOW;

void NvBlastProfilerSetCallback(physx::PxProfilerCallback* pcb)
{
	sCallback = pcb;
}

void NvBlastProfilerEnablePlatform(bool enable)
{
	sPlatform = enable;
}

void NvBlastProfilerBegin(const char* name, NvBlastProfilerDetail::Level level)
{
	if (level <= sDetail)
	{
		if (sCallback != nullptr)
		{
			sCallback->zoneStart(name, false, blastContextId);
		}

		if (sPlatform)
		{
			platformZoneStart(name);
		}
	}
}

void NvBlastProfilerEnd(const char* name, NvBlastProfilerDetail::Level level)
{
	if (level <= sDetail)
	{
		if (sCallback != nullptr)
		{
			sCallback->zoneEnd(nullptr, name, false, blastContextId);
		}

		if (sPlatform)
		{
			platformZoneEnd(name);
		}
	}
}

void NvBlastProfilerSetDetail(NvBlastProfilerDetail::Level level)
{
	sDetail = level;
}

#else

void NvBlastProfilerSetCallback(physx::PxProfilerCallback*) {}
void NvBlastProfilerEnablePlatform(bool) {}
void NvBlastProfilerSetDetail(NvBlastProfilerDetail::Level) {}

#endif //NV_PROFILE
