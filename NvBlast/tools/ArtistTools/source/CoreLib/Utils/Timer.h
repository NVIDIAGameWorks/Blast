// This code contains NVIDIA Confidential Information and is disclosed 
// under the Mutual Non-Disclosure Agreement. 
// 
// Notice 
// ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES 
// NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO 
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT, 
// MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. 
// 
// NVIDIA Corporation assumes no responsibility for the consequences of use of such 
// information or for any infringement of patents or other rights of third parties that may 
// result from its use. No license is granted by implication or otherwise under any patent 
// or patent rights of NVIDIA Corporation. No third party distribution is allowed unless 
// expressly authorized by NVIDIA.  Details are subject to change without notice. 
// This code supersedes and replaces all information previously supplied. 
// NVIDIA Corporation products are not authorized for use as critical 
// components in life support devices or systems without express written approval of 
// NVIDIA Corporation. 
// 
// Copyright (c) 2013 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "corelib_global.h"

class CORELIB_EXPORT Timer
{
public:
	Timer(bool isStartNow = false);
	void Start();
	void Reset(bool isStartNow = false);
	void Pause();
	bool IsRunning() const
	{
		return !m_isPaused;
	}
	double GetTimeInMilliSeconds() const
	{
		return double(GetTicksElapsed()*1000.0) / GetTicksPerSecond();
	}
	double GetTimeInSeconds() const
	{
		return double(GetTicksElapsed()) / GetTicksPerSecond();
	}

public:
	bool m_isPaused;

	// Get time ticks elapsed
	LONGLONG GetTicksElapsed() const;
	// Get the timer frequency. Time tick count per second
	LONGLONG GetTicksPerSecond() const {
		return m_ticksPerSecond.QuadPart;
	}
	LARGE_INTEGER m_ticksPerSecond;
	LARGE_INTEGER m_lastPausedTime;
	LARGE_INTEGER m_totalPausedTime;
};
