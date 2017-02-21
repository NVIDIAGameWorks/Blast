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

#include "MathUtil.h"

#define MAX_LIGHTS 4

// shader parameter for light
struct LightParam
{
	int				m_enable;
	atcore_float3	m_dir;

	int				m_useShadows;
	atcore_float3	m_color;

	atcore_float3	m_ambientColor;
	int				m_isEnvLight;

	int				m_lhs;
	int				_reserved1;
	int				_reserved2;
	int				_reserved3;

	float			m_depthBias;
	float			m_depthGain;
	int				m_useEnvMap;
	float			m_intensity;

	atcore_float4x4	m_viewMatrix;
	atcore_float4x4	m_lightMatrix;

public:
	LightParam()
	{
		m_dir = gfsdk_makeFloat3(-1.0f, -1.0f, -1.0f);
		m_enable	= 0;
		m_useShadows = false;
		m_isEnvLight = 0;
		m_useEnvMap = 0;

		m_depthBias = 1.0f;
		m_depthGain = 1.0f;

		m_color = gfsdk_makeFloat3(1.0f, 1.0f, 1.0f);
		m_ambientColor = gfsdk_makeFloat3(0.0f, 0.0f, 0.0f);
	}
};


// light shader block in c-buffer 
struct LightShaderParam
{
	LightParam		m_lightParam[MAX_LIGHTS];
};

