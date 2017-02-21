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
#include "LightShaderParam.h"

// same layout as the constant buffer used in body shader
#define MAX_BONE_MATRICES 512

// shared struct for mesh shader cbuffer and material access
struct MeshShaderParam
{
	atcore_float4x4	m_ViewProjection;
	atcore_float4x4	m_BodyTransformation;

	LightShaderParam m_lightParam;

	atcore_float3	m_eyePosition;
	float			m_specularShininess;

	int				m_useDiffuseTextures;
	int				m_useSpecularTextures;
	int				m_useNormalTextures;
	int				m_useTextures;


	atcore_float4	m_ambientColor;
	atcore_float4	m_diffuseColor;
	atcore_float4	m_specularColor;

	int				m_wireFrame;
	int				m_useLighting;
	int				m_wireFrameOver;
	float			m_unitScale;

	int				m_useDQs;
	int				m_diffuseChannel;
	int				m_flatNormal;
	int 			m_usePinPos;

	atcore_float4x4		 m_boneMatrices[MAX_BONE_MATRICES];
	atcore_dualquaternion m_boneDQs[MAX_BONE_MATRICES];

	MeshShaderParam()
	{
		m_specularShininess			= 30.0f;

		m_ambientColor				= gfsdk_makeFloat4(0.0f, 0.0f, 0.0f, 1.0f);
		m_diffuseColor				= gfsdk_makeFloat4(1.0f, 1.0f, 1.0f, 1.0f);
		m_specularColor				= gfsdk_makeFloat4(0.0f, 0.0f, 0.0f, 0.0f);

		m_useDiffuseTextures		= true;
		m_useSpecularTextures		= true;
		m_useNormalTextures			= true;
		m_useTextures				= true;

		m_wireFrame					= false;
		m_wireFrameOver				= false;
		m_useLighting				= true;
		m_unitScale					= 1.0f;

		m_useDQs					= false;
		m_flatNormal				= false;

		m_usePinPos					= false;

		memset(m_boneMatrices, 0, sizeof(atcore_float4x4) * MAX_BONE_MATRICES);
		memset(m_boneDQs, 0, sizeof(atcore_dualquaternion) * MAX_BONE_MATRICES);
	}
};

// struct for mesh shadow shader cbuffer 
struct MeshShadowShaderParam
{
	atcore_float4x4	m_ViewProjection;
	atcore_float4x4	m_ViewMatrix;
	atcore_float4x4	m_BodyTransformation;

	int				m_useDQs;
	int 			m_usePinPos;
	float			_reserved_[2];

	atcore_float4x4		 m_boneMatrices[MAX_BONE_MATRICES];
	atcore_dualquaternion m_boneDQs[MAX_BONE_MATRICES];

	MeshShadowShaderParam()
	{
		m_useDQs = false;
		m_usePinPos = false;

		memset(m_boneMatrices, 0, sizeof(atcore_float4x4) * MAX_BONE_MATRICES);
		memset(m_boneDQs, 0, sizeof(atcore_dualquaternion) * MAX_BONE_MATRICES);
	}
};

struct SimpleShaderParam
{
	atcore_float4x4 world;
	atcore_float4x4 view;
	atcore_float4x4 projection;
	atcore_float4	color;

	int		useVertexColor;
	int		dummy2;
	int		dummy3;
	int		dummy4;
};

struct ShadowVizParam
{
	float			m_zNear;
	float			m_zFar;
	float			_align1;
	float			_align2;
};