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

//#define USE_GFSDK_SHADOWLIB

///////////////////////////////////////////////////////////////////////////////////
// Textures
///////////////////////////////////////////////////////////////////////////////////
Buffer<float4>			g_BoneIndices : register(t0);
Buffer<float4>			g_BoneWeights : register(t1);

Texture2D				g_DiffuseTexture : register(t2);
Texture2D				g_SpecularTexture : register(t3);
Texture2D				g_NormalTexture : register(t4);
Texture2D				g_EnvTexture : register(t5);

Texture2D				g_ShadowTexture0 : register(t6);
Texture2D				g_ShadowTexture1 : register(t7);
Texture2D				g_ShadowTexture2 : register(t8);
Texture2D				g_ShadowTexture3 : register(t9);

#include "Light.hlsl"
#include "BodyShaderCommon.hlsl"

///////////////////////////////////////////////////////////////////////////////////
// constant buffer
///////////////////////////////////////////////////////////////////////////////////
cbuffer cbPerFrame : register(b0)
{
	row_major float4x4	g_ViewProjection;
	row_major float4x4	g_BodyTransformation;

//////////////////////////////////////////////////
	Light				g_Light[4];

//////////////////////////////////////////////////
	float3				g_eyePosition;
	float				g_specularShininess;

	int					g_useDiffuseTextures;
	int					g_useSpecularTextures;
	int					g_useNormalTextures;
	int					g_useTextures;


	float4				g_ambientColor;
	float4				g_diffuseColor;
	float4				g_specularColor;

	int					g_wireFrame;
	int					g_useLighting;
	int					g_wireFrameOver;
	float				g_unitScale;

	int					g_useDQs;
	int					g_diffuseChannel;
	int					g_flatNormal;
	int 				g_usePinPos;

	row_major float4x4	g_boneMatrices[MAX_BONE_MATRICES];
	DQ					g_boneDQs[MAX_BONE_MATRICES];
}

#define FLT_EPSILON 1e-7

SamplerState samLinear : register(s0);
SamplerState samPointClamp : register(s1);

//////////////////////////////////////////////////////////////////////////////
// shadow sampling functions
//////////////////////////////////////////////////////////////////////////////
float softDepthCmp(float sampledDepth, float calcDepth, float bias, float gain)
{
	float delta = gain * (sampledDepth - (calcDepth + bias));

	float s = clamp(1.0 - delta / abs(bias), 0.0f, 1.0f);
	return s;
}

float softDepthCmpRHS(float sampledDepth, float calcDepth)
{
	float bias = g_unitScale;
	
	float delta = sampledDepth - (calcDepth + bias);

	float s = clamp(1.0 - delta / bias, 0.0f, 1.0f);
	return s;
}

float softDepthCmpLHS(float sampledDepth, float calcDepth)
{
	float bias = g_unitScale;
	
	float delta = (calcDepth - bias) - sampledDepth;

	float s = clamp(1.0 - delta / bias, 0.0f, 1.0f);
	return s;
}

float ShadowPCF(float2 texcoord, float calcDepth, float bias, float gain, Texture2D shadowTexture)
{
	float shadow = 0;
	float n = 0;

	[unroll]
	for (int dx = - 3; dx <= 3; dx += 2) {
		for (int dy = -3; dy <= 3; dy += 2) {
		    float4 S = shadowTexture.Gather(samPointClamp, texcoord, int2(dx, dy));
			shadow += softDepthCmp(S.x, calcDepth, bias, gain);
			shadow += softDepthCmp(S.y, calcDepth, bias, gain);
			shadow += softDepthCmp(S.z, calcDepth, bias, gain);
			shadow += softDepthCmp(S.w, calcDepth, bias, gain);
			n += 4;
		}
	}
	 
	return shadow / n;
}

float getShadow(float3 wPos, Light L, Texture2D stex)
{
	float2 texcoords = mul(float4(wPos, 1), L.m_lightMatrix).xy;
	float z = mul(float4(wPos, 1), L.m_viewMatrix).z;

	float bias = L.m_depthBias;
	float gain = L.m_depthGain;
	float shadow = ShadowPCF(texcoords, z, bias, gain, stex);

	return shadow;
}

//////////////////////////////////////////////////////////////////////////////
inline float getIllumination(Light L, float3 wPos, Texture2D stex)
{
	float lit = 1.0f;

	if (L.m_useShadows)
	{
		float2 texcoords = mul(float4(wPos, 1), L.m_lightMatrix).xy;
		float z = mul(float4(wPos, 1), L.m_viewMatrix).z;
		lit = getShadow(wPos, L, stex);
	}

	return lit;
}


/////////////////////////////////////////////////////////////////////////////////////
inline float3 computeDiffuseLighting(
	float3 I,
	float3 L, // light direction
	float3 N // surface normal
	)
{
	float diffuse = max(0, dot(N, L));

	return diffuse * I;
}

/////////////////////////////////////////////////////////////////////////////////////
inline float3 computeSpecularLighting(
	float3 I, // light color
	float3 L, // light direction
	float3 N, // surface normal
	float3 E, // view vector
	float3 Ms, // specularity

	float shininess)
{
	float3 H = normalize(E+N);
	float NdotH = max(0, dot(H, N));
	float specular	= pow(NdotH, shininess);

	float3 output = specular * I * Ms;

	return output;
}

/////////////////////////////////////////////////////////////////////////////////////
struct BodyRenderVSIn
{
	float3 Position			: POSITION;
    float3 vertexNormal		: VERTEX_NORMAL;
	float3 faceNormal		: FACE_NORMAL;
	float3 Tangent			: TANGENT;
	float2 texCoord			: TEXCOORD;
	float  vid				: VERTEX_ID;
};

struct BodyRenderVSOut
{
    float4 Position : SV_Position;
	float3 Normal   : Normal;
    float3 Tangent  : TANGENT;
    float3 wpos		: WPOS;
	float2 texCoord : TEXCOORD;
};

/////////////////////////////////////////////////////////////////////////////////////
// vertex shader
/////////////////////////////////////////////////////////////////////////////////////
BodyRenderVSOut vs_main(BodyRenderVSIn vertexIn)
{
   BodyRenderVSOut vertex;

	float3 pos = vertexIn.Position.xyz;

	float3 normal = g_flatNormal ? normalize(vertexIn.faceNormal) : normalize(vertexIn.vertexNormal);
	float3 tangent = normalize(vertexIn.Tangent);

	float3 skinnedPos, skinnedNormal, skinnedTangent;
    
	float4 boneIndex = g_BoneIndices.Load(vertexIn.vid);
	float4 boneWeight = g_BoneWeights.Load(vertexIn.vid);

	if (g_useDQs)
		computeSkinningDQ(boneIndex, boneWeight, g_boneDQs, skinnedPos, skinnedNormal, skinnedTangent, pos, normal, tangent);
	else
		computeSkinningLinear(boneIndex, boneWeight, g_boneMatrices, skinnedPos, skinnedNormal, skinnedTangent, pos, normal, tangent);
	
	if (!g_usePinPos)
		pos = skinnedPos;
 	pos = mul(float4(pos, 1), g_BodyTransformation); 
	vertex.wpos				= pos;

    vertex.Position			= mul(float4(pos, 1), g_ViewProjection);

    vertex.Normal			= normalize(skinnedNormal);
	vertex.Tangent			= normalize(skinnedTangent);
	vertex.texCoord			= vertexIn.texCoord;

	return vertex;
}

/////////////////////////////////////////////////////////////////////////////////////
// pixel shader
/////////////////////////////////////////////////////////////////////////////////////

float4 ps_main(BodyRenderVSOut vertex) : SV_Target
{      
	float4 output = float4(0,0,0,1);

	if (g_wireFrameOver)
		return output;
	
	float3 diffuseColor = g_diffuseColor.xyz;
	if (g_useDiffuseTextures)
	{
		if (g_diffuseChannel == 0)
			diffuseColor.xyz = g_DiffuseTexture.SampleLevel(samLinear,vertex.texCoord, 0).xyz;
		else if (g_diffuseChannel == 1)
			diffuseColor.xyz = g_DiffuseTexture.SampleLevel(samLinear,vertex.texCoord, 0).rrr;
		else if (g_diffuseChannel == 2)
			diffuseColor.xyz = g_DiffuseTexture.SampleLevel(samLinear,vertex.texCoord, 0).ggg;
		else if (g_diffuseChannel == 3)
			diffuseColor.xyz = g_DiffuseTexture.SampleLevel(samLinear,vertex.texCoord, 0).bbb;
		else if (g_diffuseChannel == 4)
			diffuseColor.xyz = g_DiffuseTexture.SampleLevel(samLinear,vertex.texCoord, 0).aaa;
	}

	float3 specularColor = g_specularColor.xyz;
	if (g_useSpecularTextures)
		specularColor.xyz = g_SpecularTexture.SampleLevel(samLinear,vertex.texCoord, 0).xyz;

	if (!g_useLighting)
		return float4(diffuseColor, 1.0f);

	float3 N = normalize(vertex.Normal.xyz);

	if (g_useNormalTextures)
	{
		float3 normalColor = g_NormalTexture.SampleLevel(samLinear,vertex.texCoord, 0).xyz;
		normalColor = (normalColor - 0.5) * 2.0f;

		float3 T = normalize(vertex.Tangent.xyz);
		float3 B = normalize(cross(T, N));

		float3 PN = N;
	
		PN += normalColor.x * T;
		PN += normalColor.y * B;
		PN += normalColor.z * N;

		N = normalize(PN);
	}

	float3 P = vertex.wpos.xyz;
	float3 E = normalize(g_eyePosition.xyz - P);
	float shininess = g_specularShininess;

	// sum all lights
	Texture2D stex[4] = 
	{
		g_ShadowTexture0,
		g_ShadowTexture1,
		g_ShadowTexture2,
		g_ShadowTexture3
	};

	float3 albedo = diffuseColor.rgb;
	float3 specularity = specularColor.rgb;

	float3 diffuse = 0;
	float3 specular = 0;
	float3 ambient = 0;

	[unroll]
	for (int i = 0; i < 4; i++)
	{
		Light L = g_Light[i];

		if (L.m_enable)
		{
			float3 Ldiffuse = 0;
			float3 Lspecular = 0;
			float3 Ldir = 0;

			if (L.m_isEnvLight)
			{
				Ldir = N;

				bool zup = true;
				float2 texcoords = GetLightTexCoord(Ldir, zup);
				float3 Lcolor = (L.m_useEnvMap) ? g_EnvTexture.SampleLevel(samLinear,texcoords.xy,0).rgb : L.m_color;
				Lcolor *= L.m_intensity;
				Ldiffuse = Lspecular = Lcolor;
			}
			else
			{
				float I = getIllumination(L, P, stex[i]);
				Ldiffuse = Lspecular = I * L.m_intensity * L.m_color;
				Ldir = 1.0f * L.m_dir;
			}

			diffuse += computeDiffuseLighting( Ldiffuse, Ldir, N);
			specular += computeSpecularLighting( Lspecular, Ldir, N, E, specularity, shininess);

			ambient += L.m_ambientColor;
		}
	}

	output.rgb = (ambient + diffuse) * albedo + specular;

	return output;
}
