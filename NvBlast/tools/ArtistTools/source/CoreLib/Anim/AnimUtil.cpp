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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This file contains wrapper functions to make hair lib easy to setup and use
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <string>

#include "AnimUtil.h"
#include "MathUtil.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AnimationCache::~AnimationCache()
{
	Release();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void AnimationCache::Initialize(int numBones, NvInt32 frameStart, NvInt32 frameEnd)
{
	m_numBones = numBones;
	m_frameStart = frameStart;
	m_frameEnd = frameEnd;

	m_numFrames = frameEnd - frameStart + 1;

	Release();
	Allocate();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void AnimationCache::Allocate()
{
	m_pBoneMatrices = new atcore_float4x4[m_numFrames * m_numBones];
//	m_pBoneNames	= new char[NV_HAIR_MAX_STRING * m_numBones];
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void AnimationCache::Release()
{
	if (m_pBoneMatrices)
		delete []m_pBoneMatrices;

	if (m_pBoneNames)
		delete []m_pBoneNames;

	m_pBoneMatrices = 0;
	m_pBoneNames = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool AnimationCache::GetInterpolationInfo(float frame, int& indexStart, int& indexEnd, float &fracFrame)
{
	if (frame < m_frameStart)
		return false;

	if (frame > m_frameEnd)
		return false;

	int startFrame = (int)frame;
	
	fracFrame = frame - startFrame;

	indexStart = (int)(frame - m_frameStart);
	indexEnd = indexStart + 1;

	if (indexEnd >= (int)m_numFrames)
	{
		indexEnd = indexStart;
		fracFrame = 0.0f;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int AnimationCache::FindBone(const char *toFind) const
{
	if (!toFind)
		return -1;
	/*
	for (int i = 0; i < m_numBones; i++)
	{
		const char* boneName = GetBoneName(i);
		if (!strcmp(boneName, toFind))
			return i;
	}
	*/
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool AnimationCache::FindBoneMapping( int numBones, const NvChar* boneNames, int* mappedBoneId) const
{
	// mappedBoneId must be pre-allocated

	for (int i = 0; i < numBones; i++)
	{
		//mappedBoneId[i] = FindBone(boneNames + i * NV_HAIR_MAX_STRING);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void BoneData::Allocate(NvUInt32 numBones)
{
	Release();

	m_numBones = numBones;

	m_pPoseMatrices		= new atcore_float4x4[numBones];
	m_pSkinMatrices		= new atcore_float4x4[numBones];
	m_pBoneMatrices		= new atcore_float4x4[numBones];

	m_pSkinDQs			= new atcore_dualquaternion[numBones];
	//m_pBoneNames		= new char[NV_HAIR_MAX_STRING * m_numBones];
	m_pMappedBoneId		= new int[numBones];

	for (int i = 0; i < numBones; i++)
	{
		gfsdk_makeIdentity(m_pPoseMatrices[i]);
		gfsdk_makeIdentity(m_pSkinMatrices[i]);
		m_pMappedBoneId[i] = -1;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void BoneData::Release()
{
	if (m_pPoseMatrices)
	{
		delete []m_pPoseMatrices;
		m_pPoseMatrices = 0;
	}

	if (m_pBoneMatrices)
	{
		delete []m_pBoneMatrices;
		m_pBoneMatrices = 0;
	}

	if (m_pSkinMatrices)
	{
		delete []m_pSkinMatrices;
		m_pSkinMatrices = 0;
	}

	if (m_pSkinDQs)
	{
		delete []m_pSkinDQs;
		m_pSkinDQs = 0;
	}

	if (m_pBoneNames)
	{
		delete []m_pBoneNames;
		m_pBoneNames = 0;
	}

	if (m_pMappedBoneId)
	{
		delete []m_pMappedBoneId;
		m_pMappedBoneId = 0;
	}

	m_numBones = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void BoneData::InitializeAnimationCache(AnimationCache* pGlobalCache, const char* nodeName)
{
	if (!pGlobalCache || !pGlobalCache->isValid())
		return;

	pGlobalCache->FindBoneMapping(m_numBones, m_pBoneNames, m_pMappedBoneId);

	m_nodeId = pGlobalCache->FindBone(nodeName);

	m_pAnimationCache = pGlobalCache;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool BoneData::Update(float frameTime, const char* rootBoneName, bool bindPose, bool zup)
{
	if (!m_pAnimationCache || !m_pAnimationCache->isValid() )
		bindPose = true;

	atcore_float4x4 model;
	gfsdk_makeIdentity(model);

	if (bindPose)
	{
		for (int i = 0; i < m_numBones; i++)
		{
			m_pBoneMatrices[i] = m_pPoseMatrices[i];
			m_pSkinMatrices[i] = model;
			m_pSkinDQs[i] = gfsdk_makeDQ(m_pSkinMatrices[i]);
		}
		return true;
	}

	int indexStart, indexEnd;
	float fracFrame = 0.0f;
	if (false == m_pAnimationCache->GetInterpolationInfo(frameTime, indexStart, indexEnd, fracFrame))
		return false;

	atcore_float4x4* pBoneStart	= m_pAnimationCache->GetNodeMatrices(indexStart);
	atcore_float4x4* pBoneEnd	= m_pAnimationCache->GetNodeMatrices(indexEnd);

	int numBones = m_numBones;

	atcore_float4x4 root;
	gfsdk_makeIdentity(root);

	if (rootBoneName)
	{
		int rootIndex = m_pAnimationCache->FindBone(rootBoneName);

		if (rootIndex >= 0)
		{
			root = gfsdk_lerp(pBoneStart[rootIndex], pBoneEnd[rootIndex], fracFrame);

			atcore_float3 lT = gfsdk_getTranslation(root);

			if (zup)
				lT.z = 0;
			else
				lT.y = 0;

			gfsdk_makeIdentity(root);
			gfsdk_setTranslation(root, -1.0f * lT);
		}
	}

	// interpolate skinning matrix
	for (int i = 0; i < numBones; i++)
	{
		atcore_float4x4 bone;

		int index = m_pMappedBoneId[i];
		if (index >= 0)
			bone = gfsdk_lerp(pBoneStart[index], pBoneEnd[index], fracFrame);
		else
			gfsdk_makeIdentity(bone);

		atcore_float4x4 pose = m_pPoseMatrices[i];

		m_pBoneMatrices[i] = bone;
		m_pSkinMatrices[i] = gfsdk_inverse(pose) * bone * root;
		m_pSkinDQs[i] = gfsdk_makeDQ(m_pSkinMatrices[i]);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MeshDesc::Allocate(NvUInt32 numVertices, NvUInt32 numTriangles)
{
	Release();

	m_NumTriangles			= numTriangles;
	m_NumVertices			= numVertices;

	m_pVertices				= new atcore_float3[numVertices];

	m_pVertexNormals		= new atcore_float3[numTriangles * 3];
	m_pFaceNormals			= new atcore_float3[numTriangles * 3];

	m_pTangents				= new atcore_float3[numTriangles * 3];

	m_pIndices = new NvUInt32[numTriangles * 3];
	m_pTexCoords			= new atcore_float2[numTriangles * 3];
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MeshDesc::Release()
{
	if (m_pVertices)
		delete []m_pVertices;
	if (m_pVertexNormals)
		delete []m_pVertexNormals;
	if (m_pFaceNormals)
		delete []m_pFaceNormals;
	if (m_pTangents)
		delete []m_pTangents;

	if (m_pIndices)
		delete []m_pIndices;
	if (m_pTexCoords)
		delete []m_pTexCoords;

	m_NumTriangles = 0;
	m_NumVertices = 0;

	m_pVertices				= nullptr;
	m_pVertexNormals		= nullptr;
	m_pFaceNormals			= nullptr;
	m_pTangents				= nullptr;

	m_pIndices				= nullptr;
	m_pTexCoords			= nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MeshDesc::UpdateNormal(bool updateVertexNormal)
{
	atcore_float3* vertexNormal = new atcore_float3[m_NumVertices];
	memset(vertexNormal, 0, sizeof(atcore_float3) * m_NumVertices);

	for (int i = 0; i < m_NumTriangles; i++)
	{
		atcore_float3 faceNormal;

		int fidx = i*3;
		int id0 = m_pIndices[fidx++];
		int id1 = m_pIndices[fidx++];
		int id2 = m_pIndices[fidx++];
		
		atcore_float3 p0 = m_pVertices[id0];
		atcore_float3 p1 = m_pVertices[id1];
		atcore_float3 p2 = m_pVertices[id2];
		atcore_float3 p01 = p1 - p0;
		atcore_float3 p02 = p2 - p0;

		faceNormal.x = p01.y * p02.z - p01.z * p02.y;
		faceNormal.y = p01.z * p02.x - p01.x * p02.z;
		faceNormal.z = p01.x * p02.y - p01.y * p02.x;

		gfsdk_normalize(faceNormal);

		m_pFaceNormals[i * 3 + 0] = faceNormal;
		m_pFaceNormals[i * 3 + 1] = faceNormal;
		m_pFaceNormals[i * 3 + 2] = faceNormal;

		vertexNormal[id0] += faceNormal;
		vertexNormal[id1] += faceNormal;
		vertexNormal[id2] += faceNormal;
	}
	
	if (updateVertexNormal)
	{
		for (int i = 0; i < m_NumVertices; ++i)
			gfsdk_normalize(vertexNormal[i]);

		for (int i = 0; i < m_NumTriangles; i++)
		{
			int fidx = i*3;
			int id0 = m_pIndices[fidx++];
			int id1 = m_pIndices[fidx++];
			int id2 = m_pIndices[fidx++];
		
			for (int k = 0; k < 3; k++)
			{
				int fidx = i*3 + k;
				int vidx = m_pIndices[fidx];
				m_pVertexNormals[fidx] = vertexNormal[vidx];
			}
		}
	}

	delete []vertexNormal;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SkinData::Allocate(NvUInt32 numBones, NvUInt32 numVertices)
{
	Release();

	m_boneData.Allocate(numBones);

	m_pBoneIndices = new atcore_float4[numVertices];
	m_pBoneWeights = new atcore_float4[numVertices];

	memset(m_pBoneIndices, 0, sizeof(atcore_float4) * numVertices);
	memset(m_pBoneWeights, 0, sizeof(atcore_float4) * numVertices);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SkinData::Release()
{
	// clear memory
	if (m_pBoneIndices)
		delete []m_pBoneIndices;
	if (m_pBoneWeights)
		delete []m_pBoneWeights;

	m_pBoneIndices	= 0;
	m_pBoneWeights	= 0;

	m_boneData.Release();
}

