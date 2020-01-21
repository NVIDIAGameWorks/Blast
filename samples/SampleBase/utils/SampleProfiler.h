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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.


#ifndef SAMPLEPROFILER_H
#define SAMPLEPROFILER_H

#include <chrono>

#if NV_PROFILE

void SampleProfilerInit();
void SampleProfilerBegin(const char* name);
void SampleProfilerEnd();
void SampleProfilerReset();

struct SampleProfilerScoped
{
	SampleProfilerScoped(const char* name)
	{
		SampleProfilerBegin(name);
	}

	~SampleProfilerScoped()
	{
		SampleProfilerEnd();
	}
};

#define PROFILER_INIT() SampleProfilerInit()
#define PROFILER_BEGIN(x) SampleProfilerBegin(x)
#define PROFILER_END() SampleProfilerEnd()
#define PROFILER_SCOPED(x) SampleProfilerScoped __scopedProfiler__(x)
#define PROFILER_SCOPED_FUNCTION() SampleProfilerScoped __scopedProfiler__(__FUNCTION__)
#define PROFILER_RESET() SampleProfilerReset()

#else

#define PROFILER_INIT()
#define PROFILER_BEGIN(x)
#define PROFILER_END()
#define PROFILER_SCOPED(x)
#define PROFILER_SCOPED_FUNCTION()
#define PROFILER_RESET()

#endif

void SampleProfilerDumpToFile(const char* path);
bool SampleProfilerIsValid();
std::chrono::microseconds SampleProfilerGetOverhead();

struct SampleProfilerTreeIterator
{
	struct Data
	{
		uint64_t					hash;
		const char*					name;
		bool						hasChilds;
		uint32_t					depth;
		std::chrono::microseconds	time;
		std::chrono::microseconds	maxTime;
		uint32_t					calls;
	};

	virtual const Data* data() const = 0;
	virtual bool isDone() const = 0;
	virtual void next() = 0;
	virtual void release() = 0;
};

SampleProfilerTreeIterator* SampleProfilerCreateTreeIterator();

#endif //SAMPLEPROFILER_H