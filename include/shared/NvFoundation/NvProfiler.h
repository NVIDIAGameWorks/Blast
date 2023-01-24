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

#ifndef NV_PROFILER_H
#define NV_PROFILER_H

#include <NvSimpleTypes.h>

namespace nvidia
{
    class NvProfilerCallback;
    namespace shdfnd
    {
        NV_FOUNDATION_API NvProfilerCallback *getProfilerCallback();
        NV_FOUNDATION_API void setProfilerCallback(NvProfilerCallback *profiler);
    }
}


namespace nvidia
{

struct NvProfileContext
{
    enum Enum
    {
        eNONE = 0 //!< value for no specific profile context. \see NvProfilerCallback::zoneAt
    };
};


/**
\brief The pure virtual callback interface for general purpose instrumentation and profiling of GameWorks modules as well as applications
*/
class NvProfilerCallback
{
protected:
	virtual ~NvProfilerCallback()	{}

public:
	/**************************************************************************************************************************
	Instrumented profiling events
	***************************************************************************************************************************/

	/**
	\brief Mark the beginning of a nested profile block
	\param[in] eventName	Event name. Must be a persistent const char *
	\param[in] detached		True for cross thread events
	\param[in] contextId	the context id of this zone. Zones with the same id belong to the same group. 0 is used for no specific group.
	\return Returns implementation-specific profiler data for this event
	*/
	virtual void* zoneStart(const char* eventName, bool detached, uint64_t contextId) = 0;

	/**
	\brief Mark the end of a nested profile block
	\param[in] profilerData	The data returned by the corresponding zoneStart call (or NULL if not available)
	\param[in] eventName	The name of the zone ending, must match the corresponding name passed with 'zoneStart'. Must be a persistent const char *.
	\param[in] detached		True for cross thread events. Should match the value passed to zoneStart.
	\param[in] contextId	The context of this zone. Should match the value passed to zoneStart.

	\note eventName plus contextId can be used to uniquely match up start and end of a zone.
	*/
	virtual void zoneEnd(void* profilerData, const char* eventName, bool detached, uint64_t contextId) = 0;
};

class NvProfileScoped
{
public:
    NV_FORCE_INLINE NvProfileScoped(const char* eventName, bool detached, uint64_t contextId)
        : mCallback(nvidia::shdfnd::getProfilerCallback())
    {
        if (mCallback)
        {
            mEventName = eventName;
            mDetached = detached;
            mContextId = contextId;
            mProfilerData = mCallback->zoneStart(mEventName, mDetached, mContextId);
        }
    }
    ~NvProfileScoped(void)
    {
        if (mCallback)
        {
            mCallback->zoneEnd(mProfilerData, mEventName, mDetached, mContextId);
        }
    }
    nvidia::NvProfilerCallback* mCallback;
    void*                       mProfilerData;
    const char*                 mEventName;
    bool                        mDetached;
    uint64_t                    mContextId;
};



} // end of NVIDIA namespace



#if NV_DEBUG || NV_CHECKED || NV_PROFILE

#define NV_PROFILE_ZONE(name,context_id) nvidia::NvProfileScoped NV_CONCAT(_scoped,__LINE__)(name,false,context_id)
#define NV_PROFILE_START_CROSSTHREAD(name,context_id) if ( nvidia::shdfnd::getProfilerCallback() ) nvidia::shdfnd::getProfilerCallback()->zoneStart(name,true,context_id)
#define NV_PROFILE_STOP_CROSSTHREAD(name,context_id) if ( nvidia::shdfnd::getProfilerCallback() ) nvidia::shdfnd::getProfilerCallback()->zoneEnd(nullptr,name,true,context_id)

#else

#define NV_PROFILE_ZONE(name,context_id) 
#define NV_PROFILE_START_CROSSTHREAD(name,context_id)
#define NV_PROFILE_STOP_CROSSTHREAD(name,context_id)

#endif

#define NV_PROFILE_POINTER_TO_U64( pointer ) static_cast<uint64_t>(reinterpret_cast<size_t>(pointer))

#endif
