/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NvBlast.h"
#include "NvBlastTime.h"
#include <cstring>


extern "C"
{

void NvBlastTimersReset(NvBlastTimers* timers)
{
	memset(timers, 0, sizeof(NvBlastTimers));
}

double NvBlastTicksToSeconds(int64_t ticks)
{
	return Nv::Blast::Time::seconds(ticks);
}

} // extern "C"
