/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

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