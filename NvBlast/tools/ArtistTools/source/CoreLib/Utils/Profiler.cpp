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

#include "nvToolsExt.h"
#include <stdlib.h> // for rand()

void ProfilerRangePush(const char* name) {
	unsigned int color = 0xFF000000 | ((rand()%256) << 16) | ((rand()%256) << 8) | (rand()%256);
	// Zero the structure
	nvtxEventAttributes_t event_attrib = {0};
	// Set the version and the size information
	event_attrib.version = NVTX_VERSION;
	event_attrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
	// Configure the attributes.  0 is the default for all attributes
	event_attrib.colorType = NVTX_COLOR_ARGB;
	event_attrib.color = color;
	event_attrib.messageType = NVTX_MESSAGE_TYPE_ASCII;
	event_attrib.message.ascii = name;
	nvtxRangePushEx(&event_attrib);
}

void ProfilerRangePush(const char* name, unsigned int color) {
	// Zero the structure
	nvtxEventAttributes_t event_attrib = {0};
	// Set the version and the size information
	event_attrib.version = NVTX_VERSION;
	event_attrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
	// Configure the attributes.  0 is the default for all attributes
	event_attrib.colorType = NVTX_COLOR_ARGB;
	event_attrib.color = color;
	event_attrib.messageType = NVTX_MESSAGE_TYPE_ASCII;
	event_attrib.message.ascii = name;
	nvtxRangePushEx(&event_attrib);
}

void ProfilerRangePop() {
	nvtxRangePop();
}

#include "Profiler.h"

ScopedProfiler::ScopedProfiler(const char* name)
{
	ProfilerRangePush(name);
}

ScopedProfiler::ScopedProfiler(const char* name, unsigned int color)
{
	ProfilerRangePush(name, color);
}

ScopedProfiler::~ScopedProfiler()
{
	ProfilerRangePop();
}
