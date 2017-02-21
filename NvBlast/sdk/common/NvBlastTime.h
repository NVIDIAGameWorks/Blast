// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTTIME_H
#define NVBLASTTIME_H

#include "NvBlastTypes.h"


namespace Nv
{
namespace Blast
{

class Time
{
public:
	Time() : m_lastTickCount(getTimeTicks()) {}

	int64_t			getElapsedTicks()
	{
		const int64_t lastTickCount = m_lastTickCount;
		m_lastTickCount = getTimeTicks();
		return m_lastTickCount - lastTickCount;
	}

	int64_t			peekElapsedTicks() const
	{
		return getTimeTicks() - m_lastTickCount;
	}

	int64_t			getLastTickCount() const
	{
		return m_lastTickCount;
	}

	static double	seconds(int64_t ticks)
	{
		return s_secondsPerTick * ticks;
	}

private:
	int64_t			getTimeTicks() const;
	static double	getTickDuration();

	int64_t				m_lastTickCount;
	static const double	s_secondsPerTick;
};

} // namespace Blast
} // namespace Nv


//////// Time inline functions for various platforms ////////

#if NV_MICROSOFT_FAMILY

#include "NvBlastIncludeWindows.h"

NV_INLINE int64_t Nv::Blast::Time::getTimeTicks() const
{
	LARGE_INTEGER a;
	QueryPerformanceCounter(&a);
	return a.QuadPart;
}

NV_INLINE double Nv::Blast::Time::getTickDuration()
{
	LARGE_INTEGER a;
	QueryPerformanceFrequency(&a);
	return 1.0 / (double)a.QuadPart;
}

#elif NV_UNIX_FAMILY

#include <time.h>

NV_INLINE int64_t Nv::Blast::Time::getTimeTicks() const
{
	struct timespec mCurrTimeInt;
	clock_gettime(CLOCK_REALTIME, &mCurrTimeInt);
	return (static_cast<int64_t>(mCurrTimeInt.tv_sec) * 1000000000) + (static_cast<int64_t>(mCurrTimeInt.tv_nsec));
}

NV_INLINE double Nv::Blast::Time::getTickDuration()
{
	return 1.e-9;
}

#elif NV_PS4

#include "ps4/NvBlastTimePS4.h"

#endif

#endif // #ifndef NVBLASTTIME_H
