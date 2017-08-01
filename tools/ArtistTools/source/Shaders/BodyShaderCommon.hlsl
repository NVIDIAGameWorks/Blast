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

struct DQ
{
	float4 q0;
	float4 q1;
};

#define MAX_BONE_MATRICES 512
#define FLT_EPSILON 1e-7

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// apply linear blending based skinning
void computeSkinningLinear(
	float4 boneIndex,
	float4 boneWeight,
	row_major float4x4	boneMatrices[MAX_BONE_MATRICES],
	out float3 skinnedPosition, 
	out float3 skinnedNormal, 
	out float3 skinnedTangent, 
	float3 restPosition, float3 restNormal, float3 restTangent)
{

	float weightSum = boneWeight.x + boneWeight.y + boneWeight.z + boneWeight.w ;
	float invWeightSum = 1.0f / (weightSum + FLT_EPSILON);

	skinnedPosition = float3(0, 0, 0);
	skinnedNormal = float3(0, 0, 0);
	skinnedTangent = float3(0, 0, 0);

	[unroll(4)]
	for (int b = 0; b < 4; b++)
	{
		row_major float4x4 bone = boneMatrices[boneIndex[b]];
		float w = boneWeight[b];

		float3 p = (mul(float4(restPosition.xyz,1), bone)).xyz;
		skinnedPosition.xyz += w * p;
		float3 n = (mul(float4(restNormal.xyz,0), bone)).xyz;
		skinnedNormal.xyz += w * n;
		float3 t = (mul(float4(restTangent.xyz,0), bone)).xyz;
		skinnedTangent.xyz += w * t;
	}

	skinnedPosition.xyz *= invWeightSum;
	skinnedNormal.xyz *= invWeightSum;
	skinnedTangent.xyz *= invWeightSum;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// apply linear blending based skinning
void computeSkinningLinear(
	float4 boneIndex,
	float4 boneWeight,
	row_major float4x4	boneMatrices[MAX_BONE_MATRICES],
	out float3 skinnedPosition, 
	float3 restPosition)
{

	float weightSum = boneWeight.x + boneWeight.y + boneWeight.z + boneWeight.w ;
	float invWeightSum = 1.0f / (weightSum + FLT_EPSILON);

	skinnedPosition = float3(0, 0, 0);

	[unroll(4)]
	for (int b = 0; b < 4; b++)
	{
		row_major float4x4 bone = boneMatrices[boneIndex[b]];

		float3 p = boneWeight[b] * invWeightSum * (mul(float4(restPosition.xyz,1), bone)).xyz;
		skinnedPosition.xyz += p;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// apply dual quaternion skinning
void computeSkinningDQ(
	float4	boneIndex,
	float4	boneWeight,
	DQ		boneDQs[MAX_BONE_MATRICES],
	inout float3 skinnedPosition, 
	inout float3 skinnedNormal, 
	inout float3 skinnedTangent, 
	float3 restPosition, 
	float3 restNormal, 
	float3 restTangent)
{
	DQ dq;
	dq.q0 = float4(0,0,0,0);
	dq.q1 = float4(0,0,0,0);

	[unroll(4)]
	for (int b = 0; b < 4; b++)
	{
		float w = boneWeight[b];
		DQ boneDQ = boneDQs[boneIndex[b]];

		boneDQ.q0 *= w;
		boneDQ.q1 *= w;

		// hemispherization
		float sign = (dot(dq.q0, boneDQ.q0) < -FLT_EPSILON) ? -1.0f: 1.0f;

		dq.q0 += sign * boneDQ.q0;
		dq.q1 += sign * boneDQ.q1;
	}

	// normalize
	float mag = dot(dq.q0, dq.q0);
	float deLen = 1.0f / sqrt(mag+FLT_EPSILON);
	dq.q0 *= deLen;
	dq.q1 *= deLen;

	// transform
	float3 d0 = dq.q0.xyz;
	float3 de = dq.q1.xyz;
	float a0 = dq.q0.w;
	float ae = dq.q1.w;

	float3 tempPos		= cross(d0, restPosition.xyz) + a0 * restPosition.xyz;
	float3 tempPos2		= 2.0f * (de * a0 - d0 * ae + cross(d0, de));
	float3 tempNormal	= cross(d0, restNormal.xyz) + a0 * restNormal.xyz;
	float3 tempTangent	= cross(d0, restTangent.xyz) + a0 * restTangent.xyz;
			
	skinnedPosition.xyz = restPosition.xyz + tempPos2 + cross(2.0f * d0, tempPos);
	skinnedNormal.xyz	= restNormal.xyz + 2.0 * cross( d0, tempNormal);
	skinnedTangent.xyz	= restTangent.xyz + 2.0 * cross( d0, tempTangent);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// apply dual quaternion skinning
void computeSkinningDQ(
	float4	boneIndex,
	float4	boneWeight,
	DQ		boneDQs[MAX_BONE_MATRICES],
	inout float3 skinnedPosition, 
	float3 restPosition)
{
	DQ dq;
	dq.q0 = float4(0,0,0,0);
	dq.q1 = float4(0,0,0,0);

	[unroll(4)]
	for (int b = 0; b < 4; b++)
	{
		float w = boneWeight[b];
		DQ boneDQ = boneDQs[boneIndex[b]];

		boneDQ.q0 *= w;
		boneDQ.q1 *= w;

		// hemispherization
		float sign = (dot(dq.q0, boneDQ.q0) < -FLT_EPSILON) ? -1.0f: 1.0f;

		dq.q0 += sign * boneDQ.q0;
		dq.q1 += sign * boneDQ.q1;
	}

	// normalize
	float mag = dot(dq.q0, dq.q0);
	float deLen = 1.0f / sqrt(mag+FLT_EPSILON);
	dq.q0 *= deLen;
	dq.q1 *= deLen;

	// transform
	float3 d0 = dq.q0.xyz;
	float3 de = dq.q1.xyz;
	float a0 = dq.q0.w;
	float ae = dq.q1.w;

	float3 tempPos		= cross(d0, restPosition.xyz) + a0 * restPosition.xyz;
	float3 tempPos2		= 2.0f * (de * a0 - d0 * ae + cross(d0, de));	
	skinnedPosition.xyz = restPosition.xyz + tempPos2 + cross(2.0f * d0, tempPos);
}



