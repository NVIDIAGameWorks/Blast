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
// Copyright (c) 2008-2014 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.

#ifndef NV_NSFOUNDATION_NSGLOBALS_H
#define NV_NSFOUNDATION_NSGLOBALS_H

#include "NvErrors.h"

namespace nvidia
{

class NvAssertHandler;
class NvErrorCallback;
class NvAllocatorCallback;
class NvProfilerCallback;

namespace shdfnd
{

// note: it's illegal to initialize the shared foundation twice without terminating in between

NV_FOUNDATION_API void initializeSharedFoundation(uint32_t version, NvAllocatorCallback&, NvErrorCallback&);
NV_FOUNDATION_API bool sharedFoundationIsInitialized();
NV_FOUNDATION_API void terminateSharedFoundation();

// number of times foundation has been init'd. 0 means never initialized, so if we wrap we go from UINT32_MAX to 1. Used
// for things that happen at most once (e.g. some warnings)
NV_FOUNDATION_API uint32_t getInitializationCount();

NV_FOUNDATION_API NvAllocatorCallback& getAllocator();
NV_FOUNDATION_API NvErrorCallback& getErrorCallback();

// on some platforms (notably 360) the CRT does non-recoverable allocations when asked for type names. Hence
// we provide a mechanism to disable this capability
NV_FOUNDATION_API void setReflectionAllocatorReportsNames(bool val);
NV_FOUNDATION_API bool getReflectionAllocatorReportsNames();


NV_FOUNDATION_API NvProfilerCallback *getProfilerCallback();
NV_FOUNDATION_API void setProfilerCallback(NvProfilerCallback *profiler);


}
}

#endif // #ifndef NV_NSFOUNDATION_NSGLOBALS_H
