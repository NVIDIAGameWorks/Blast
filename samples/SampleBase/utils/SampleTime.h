/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef SAMPLE_TIME_H
#define SAMPLE_TIME_H

#include <stdint.h>

class Time
{
public:
	Time() : m_lastTickCount(getTimeTicks()) {}

	double Time::getElapsedSeconds()
	{
		const int64_t lastTickCount = m_lastTickCount;
		m_lastTickCount = getTimeTicks();
		return (m_lastTickCount - lastTickCount) * s_secondsPerTick;
	}

	double Time::peekElapsedSeconds() const
	{
		return (getTimeTicks() - m_lastTickCount) * s_secondsPerTick;
	}

	double Time::getLastTime() const
	{
		return m_lastTickCount * s_secondsPerTick;
	}

private:
	static double	getTickDuration()
	{
		LARGE_INTEGER a;
		QueryPerformanceFrequency(&a);
		return 1.0 / (double)a.QuadPart;
	}

	int64_t getTimeTicks() const
	{
		LARGE_INTEGER a;
		QueryPerformanceCounter(&a);
		return a.QuadPart;
	}

	int64_t				m_lastTickCount;
	static const double	s_secondsPerTick;
};


#endif