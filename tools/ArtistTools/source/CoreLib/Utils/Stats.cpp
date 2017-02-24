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
///////////////////////////////////////////////////////////////////////////////
#include "Stats.h"

void Stats::reset()
{
	m_totalRenderTime = 0.0f;
	m_shadowRenderTime = 0.0f;
	m_meshRenderTime = 0.0f;
	m_hairRenderTime = 0.0f;
	m_hairStatsTime = 0.0f;
	m_totalUpdateTime = 0.0f;
	m_meshSkinningTime = 0.0f;
	m_hairSkinningTime = 0.0f;
	m_hairSimulationTime = 0.0f;
	m_queryTime = 0.0f;
}

///////////////////////////////////////////////////////////////////////////////
void Stats::add(const Stats& toAdd)
{
	m_totalRenderTime += toAdd.m_totalRenderTime;
	m_shadowRenderTime += toAdd.m_shadowRenderTime;
	m_meshRenderTime += toAdd.m_meshRenderTime;
	m_hairRenderTime += toAdd.m_hairRenderTime;
	m_hairStatsTime += toAdd.m_hairStatsTime;

	m_totalUpdateTime += toAdd.m_totalUpdateTime;
	m_meshSkinningTime += toAdd.m_meshSkinningTime;
	m_hairSkinningTime += toAdd.m_hairSkinningTime;
	m_hairSimulationTime += toAdd.m_hairSimulationTime;

	m_queryTime += toAdd.m_queryTime;
}

///////////////////////////////////////////////////////////////////////////////
void Stats::average(float numFrames)
{
	m_totalRenderTime /= numFrames;
	m_shadowRenderTime /= numFrames;
	m_meshRenderTime /= numFrames;
	m_hairRenderTime /= numFrames;
	m_hairStatsTime /= numFrames;
	m_totalUpdateTime /= numFrames;
	m_meshSkinningTime /= numFrames;
	m_hairSkinningTime /= numFrames;
	m_hairSimulationTime /= numFrames;
	m_queryTime /= numFrames;
}

