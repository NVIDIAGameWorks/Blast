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


#ifndef NVBLASTDEFAULTPROFILER_H
#define NVBLASTDEFAULTPROFILER_H

#include "NvBlastProfiler.h"
#include "PxProfiler.h"

#if NV_NVTX  
#include "nvToolsExt.h"
NV_INLINE void platformZoneStart(const char* name) { nvtxRangePushA(name); }
NV_INLINE void platformZoneEnd() { nvtxRangePop(); }

#elif NV_XBOXONE
#include "xboxone/NvBlastProfilerXB1.h"

#elif NV_PS4
#include "ps4/NvBlastProfilerPS4.h"

#else
NV_INLINE void platformZoneStart(const char*) { }
NV_INLINE void platformZoneEnd() { }

#endif

#define SUPPORTS_THREAD_LOCAL (!NV_VC || NV_VC > 12)

namespace Nv
{
namespace Blast
{

struct ExtProfileData
{
	const char* name;
	void* data;
};

#if SUPPORTS_THREAD_LOCAL
static const int32_t PROFILER_MAX_NESTED_DEPTH = 64;
static thread_local ExtProfileData th_ProfileData[PROFILER_MAX_NESTED_DEPTH];
static thread_local int32_t th_depth = 0;
#endif


/**
Implements Nv::Blast::ProfilerCallback to serve the physx::PxProfilerCallback set in PxFoundation
for PhysX Visual Debugger support and platform specific profilers like NVIDIA(R) NSight(TM).
*/
class ExtCustomProfiler : public ProfilerCallback
{
public:
	/**
	Construct an ExtCustomProfiler with platform specific profiler signals disabled.
	*/
	ExtCustomProfiler() : m_platformEnabled(false) {}


	////// ProfilerCallback interface //////

	virtual void zoneStart(const char* name) override
	{

#if SUPPORTS_THREAD_LOCAL
		if (PxGetProfilerCallback())
		{
			void* data = PxGetProfilerCallback()->zoneStart(name, false, 0xb1a57);

			if (th_depth < PROFILER_MAX_NESTED_DEPTH && th_depth >= 0)
			{
				th_ProfileData[th_depth].name = name;
				th_ProfileData[th_depth].data = data;
				th_depth++;
			}
			else
			{
				assert(th_depth < PROFILER_MAX_NESTED_DEPTH && th_depth >= 0);
			}
		}
#endif

		if (m_platformEnabled)
		{
			platformZoneStart(name);
		}
	}

	virtual void zoneEnd() override
	{

#if SUPPORTS_THREAD_LOCAL
		if (PxGetProfilerCallback())
		{
			th_depth--;

			if (th_depth >= 0)
			{
				ExtProfileData& pd = th_ProfileData[th_depth];
				PxGetProfilerCallback()->zoneEnd(pd.data, pd.name, false, 0xb1a57);
			}
			else
			{
				assert(th_depth >= 0);
			}
		}
#endif

		if (m_platformEnabled)
		{
			platformZoneEnd();
		}
	}


	////// local interface //////

	/**
	Enable or disable platform specific profiler signals. Disabled by default.

	\param[in]	enabled		true enables, false disables platform profiler calls.
	*/
	void setPlatformEnabled(bool enabled)
	{
		m_platformEnabled = enabled;
	}

private:
	bool m_platformEnabled;
};

} // namespace Blast
} // namespace Nv


#endif // NVBLASTDEFAULTPROFILER_H
