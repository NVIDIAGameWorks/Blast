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
// Copyright (c) 2013-2015 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//

#pragma once

#include "corelib_global.h"

struct CORELIB_EXPORT GPUProfiler
{
	bool	m_enable;

public:
	// define base class virtual destructor to make sure child classes call destructor right.
	virtual ~GPUProfiler() {}

	void Enable(bool b) { m_enable = b; }

	virtual void Initialize() = 0;
	virtual void Release() = 0;
	virtual void StartFrame() = 0;
	virtual void EndFrame() = 0;
	virtual void StartProfile(int id) = 0;
	virtual void EndProfile(int id) = 0;
	virtual float GetProfileData(int id) = 0;
};

// helper functions
CORELIB_EXPORT void GPUProfiler_Initialize();
CORELIB_EXPORT void GPUProfiler_Enable(bool);
CORELIB_EXPORT void GPUProfiler_Shutdown();
CORELIB_EXPORT void GPUProfiler_StartFrame();
CORELIB_EXPORT void GPUProfiler_EndFrame();
CORELIB_EXPORT void GPUProfiler_StartProfile(int id);
CORELIB_EXPORT void GPUProfiler_EndProfile(int id);
CORELIB_EXPORT float GPUProfiler_GetProfileData(int id);

