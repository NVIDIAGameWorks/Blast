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
// Copyright (c) 2016-2020 NVIDIA Corporation. All rights reserved.


#include "stdint.h"
#include "NvBlastProfilerInternal.h"

#if NV_PROFILE || NV_CHECKED || NV_DEBUG

namespace Nv
{
namespace Blast
{

class EmptyProfilerCallback : public ProfilerCallback
{
	void zoneStart(const char*) {}
	void zoneEnd() {}
};
EmptyProfilerCallback g_EmptyCallback;

ProfilerCallback* g_ProfilerCallback = &g_EmptyCallback;
ProfilerDetail::Level g_ProfilerDetail = ProfilerDetail::LOW;

} // namespace Blast
} // namespace Nv 


void NvBlastProfilerSetCallback(Nv::Blast::ProfilerCallback* pcb)
{
	Nv::Blast::g_ProfilerCallback = pcb != nullptr ? pcb : &Nv::Blast::g_EmptyCallback;
}

Nv::Blast::ProfilerCallback* NvBlastProfilerGetCallback()
{
	return Nv::Blast::g_ProfilerCallback;
}


void NvBlastProfilerSetDetail(Nv::Blast::ProfilerDetail::Level level)
{
	Nv::Blast::g_ProfilerDetail = level;
}

Nv::Blast::ProfilerDetail::Level NvBlastProfilerGetDetail()
{
	return Nv::Blast::g_ProfilerDetail;
}


void NvBlastProfilerBegin(const char* name, Nv::Blast::ProfilerDetail::Level level)
{
	if (level <= NvBlastProfilerGetDetail())
	{
		NvBlastProfilerGetCallback()->zoneStart(name);
	}
}

void NvBlastProfilerEnd(const void* /*name*/, Nv::Blast::ProfilerDetail::Level level)
{
	if (level <= NvBlastProfilerGetDetail())
	{
		NvBlastProfilerGetCallback()->zoneEnd();
	}
}

#else

void NvBlastProfilerSetCallback(Nv::Blast::ProfilerCallback*) {}
void NvBlastProfilerSetDetail(Nv::Blast::ProfilerDetail::Level) {}

#endif // NV_PROFILE
