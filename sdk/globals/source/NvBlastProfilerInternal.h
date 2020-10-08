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


#ifndef NVBLASTPROFILERINTERNAL_H
#define NVBLASTPROFILERINTERNAL_H

#include "NvBlastPreprocessor.h"
#include "NvBlastProfiler.h"

#if NV_PROFILE || NV_CHECKED || NV_DEBUG

NVBLAST_API void NvBlastProfilerBegin(const char* name, Nv::Blast::ProfilerDetail::Level);
NVBLAST_API void NvBlastProfilerEnd(const void* name, Nv::Blast::ProfilerDetail::Level);

Nv::Blast::ProfilerCallback* NvBlastProfilerGetCallback();
Nv::Blast::ProfilerDetail::Level NvBlastProfilerGetDetail();


namespace Nv
{
namespace Blast
{

	
class ProfileScope
{
public:
	ProfileScope(const char* name, ProfilerDetail::Level level) :m_name(name), m_level(level)
	{
		NvBlastProfilerBegin(m_name, m_level);
	}

	~ProfileScope()
	{
		NvBlastProfilerEnd(m_name, m_level);
	}

private:
	const char* m_name;
	ProfilerDetail::Level m_level;
};


} // namespace Blast
} // namespace Nv


#define BLAST_PROFILE_PREFIX				"Blast: "
#define BLAST_PROFILE_ZONE_BEGIN(name)		NvBlastProfilerBegin(BLAST_PROFILE_PREFIX name, Nv::Blast::ProfilerDetail::HIGH)
#define BLAST_PROFILE_ZONE_END(name)		NvBlastProfilerEnd(BLAST_PROFILE_PREFIX name, Nv::Blast::ProfilerDetail::HIGH)
#define BLAST_PROFILE_SCOPE(name, detail)	Nv::Blast::ProfileScope NV_CONCAT(_scope,__LINE__) (BLAST_PROFILE_PREFIX name, detail)
#define BLAST_PROFILE_SCOPE_L(name)			BLAST_PROFILE_SCOPE(name, Nv::Blast::ProfilerDetail::LOW)
#define BLAST_PROFILE_SCOPE_M(name)			BLAST_PROFILE_SCOPE(name, Nv::Blast::ProfilerDetail::MEDIUM)
#define BLAST_PROFILE_SCOPE_H(name)			BLAST_PROFILE_SCOPE(name, Nv::Blast::ProfilerDetail::HIGH)

#else

#define BLAST_PROFILE_ZONE_BEGIN(name)	
#define BLAST_PROFILE_ZONE_END(name)
#define BLAST_PROFILE_SCOPE_L(name)
#define BLAST_PROFILE_SCOPE_M(name)
#define BLAST_PROFILE_SCOPE_H(name)

#endif

#endif
