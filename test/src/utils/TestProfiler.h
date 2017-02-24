#ifndef TESTPROFILER_H
#define TESTPROFILER_H

#include "NvBlastPreprocessor.h"

#if NV_NVTX  
#include "nvToolsExt.h"
NV_INLINE void platformZoneStart(const char* name) { nvtxRangePush(name); }
NV_INLINE void platformZoneEnd(const char*) { nvtxRangePop(); }

#elif NV_XBOXONE
#define NOMINMAX
#include "xboxone/NvBlastProfilerXB1.h"

#elif NV_PS4
#include "ps4/NvBlastProfilerPS4.h"

#else
NV_INLINE void platformZoneStart(const char*) { }
NV_INLINE void platformZoneEnd(const char*) { }

#endif

#define TEST_ZONE_BEGIN(name) platformZoneStart(name)
#define TEST_ZONE_END(name)   platformZoneEnd(name)

#endif // TESTPROFILER_H
