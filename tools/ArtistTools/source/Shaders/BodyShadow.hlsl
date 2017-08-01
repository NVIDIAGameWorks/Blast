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

Buffer<float4>			g_BoneIndices : register(t0);
Buffer<float4>			g_BoneWeights : register(t1);

#include "BodyShaderCommon.hlsl"

///////////////////////////////////////////////////////////////////////////////////
// constant buffer
///////////////////////////////////////////////////////////////////////////////////
cbuffer cbPerFrame : register(b0)
{
	row_major float4x4	g_ViewProjection;
	row_major float4x4	g_ViewMatrix;
	row_major float4x4	g_BodyTransformation;

	int					g_useDQs;
	int 				g_usePinPos;
	float				_reserved2;
	float				_reserved3;

	row_major float4x4	g_boneMatrices[MAX_BONE_MATRICES];
	DQ					g_boneDQs[MAX_BONE_MATRICES];
}

struct VSIn
{
	float3 Position : POSITION;
    float3 vertexNormal		: VERTEX_NORMAL;
	float3 faceNormal		: FACE_NORMAL;
	float3 Tangent   : TANGENT;
	float2 texCoord : TEXCOORD;
	float  vid		: VERTEX_ID;
};

struct VSOut
{
    float4 Position : SV_Position;
    float Depth : texcoord;
};

struct PSOut
{
	float Z: SV_Target0;
};

float WorldToDepth(float3 wPos)
{
	float z = mul(float4(wPos, 1), g_ViewMatrix).z;
    return z;
    	
}

VSOut vs_main(VSIn vertexIn)
{
	VSOut vertex;
	
	float4 boneIndex = g_BoneIndices.Load(vertexIn.vid);
	float4 boneWeight = g_BoneWeights.Load(vertexIn.vid);

	float3 pos = vertexIn.Position.xyz;
	float3 skinnedPos;

	if (g_useDQs)
		computeSkinningDQ(boneIndex, boneWeight, g_boneDQs, skinnedPos, pos);
	else
   		computeSkinningLinear(boneIndex, boneWeight, g_boneMatrices, skinnedPos, pos);

	if (g_usePinPos)
		skinnedPos = pos;

	float3 wPos =  mul(float4(skinnedPos, 1), g_BodyTransformation).xyz; 
    vertex.Position = mul(float4(wPos, 1), g_ViewProjection);
    vertex.Depth = WorldToDepth(wPos);
    return vertex;
}

PSOut ps_main(VSOut vertex)
{
	PSOut output;
	output.Z = vertex.Depth;
	return output;
}
