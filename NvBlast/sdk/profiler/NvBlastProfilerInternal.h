/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTPROFILERINTERNAL_H
#define NVBLASTPROFILERINTERNAL_H

#include "NvBlastPreprocessor.h"
#include "NvBlastProfiler.h"

#if NV_PROFILE || NV_CHECKED || NV_DEBUG

void NvBlastProfilerBegin(const char* name, NvBlastProfilerDetail::Level);
void NvBlastProfilerEnd(const char* name, NvBlastProfilerDetail::Level);

class ProfileScope
{
public:
	ProfileScope(const char* name, NvBlastProfilerDetail::Level level) :m_name(name), m_level(level)
	{
		NvBlastProfilerBegin(m_name, m_level);
	}

	~ProfileScope() 
	{
		NvBlastProfilerEnd(m_name, m_level);
	}

private:
	const char* m_name;
	NvBlastProfilerDetail::Level m_level;
};

#define PERF_BLAST_PREFIX			"Blast: "
#define PERF_ZONE_BEGIN(name)		NvBlastProfilerBegin(PERF_BLAST_PREFIX name, NvBlastProfilerDetail::HIGH)
#define PERF_ZONE_END(name)			NvBlastProfilerEnd(PERF_BLAST_PREFIX name, NvBlastProfilerDetail::HIGH)
#define PERF_SCOPE(name, detail)	ProfileScope PX_CONCAT(_scope,__LINE__) (PERF_BLAST_PREFIX name, detail)
#define PERF_SCOPE_L(name)			PERF_SCOPE(name, NvBlastProfilerDetail::LOW)
#define PERF_SCOPE_M(name)			PERF_SCOPE(name, NvBlastProfilerDetail::MEDIUM)
#define PERF_SCOPE_H(name)			PERF_SCOPE(name, NvBlastProfilerDetail::HIGH)

#else

#define PERF_ZONE_BEGIN(name)	
#define PERF_ZONE_END(name)
#define PERF_SCOPE_L(name)
#define PERF_SCOPE_M(name)
#define PERF_SCOPE_H(name)

#endif

#endif
