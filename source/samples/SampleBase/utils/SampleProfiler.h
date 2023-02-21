// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2008-2023 NVIDIA Corporation. All rights reserved.


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
        uint64_t                    hash;
        const char*                 name;
        bool                        hasChilds;
        uint32_t                    depth;
        std::chrono::microseconds   time;
        std::chrono::microseconds   maxTime;
        uint32_t                    calls;
    };

    virtual const Data* data() const = 0;
    virtual bool isDone() const = 0;
    virtual void next() = 0;
    virtual void release() = 0;
};

SampleProfilerTreeIterator* SampleProfilerCreateTreeIterator();

#endif //SAMPLEPROFILER_H