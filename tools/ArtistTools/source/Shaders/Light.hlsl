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

///////////////////////////////////////////////////////////////////////////////////
struct Light 
{
	int				m_enable;
	float3			m_dir;

	int				m_useShadows;
	float3			m_color;

	float3			m_ambientColor;
	int				m_isEnvLight;

	int				m_lhs;
	int				_reserved1;
	int				_reserved2;
	int				_reserved3;

	float			m_depthBias;
	float			m_depthGain;
	int				m_useEnvMap;
	float			m_intensity;

	row_major float4x4	m_viewMatrix;
	row_major float4x4	m_lightMatrix;
};

float2 GetLightTexCoord(float3 Ldir, bool zup = true)
{
	const float M_PI = 3.1415923;

	float coord0 = zup ? Ldir.x : Ldir.x;
	float coord1 = zup ? Ldir.y : Ldir.z;
	float coord2 = zup ? Ldir.z : Ldir.y;

	float u = 0.5f + 0.5f * atan2(coord1, coord0) / M_PI;
	float v = 0.5f - 0.5f * coord2;

	return float2(u,v);
}


