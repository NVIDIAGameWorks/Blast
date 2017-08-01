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

#include "MathUtil.h"
#include <string.h>
#define NV_HAIR_MAX_STRING 128

////////////////////////////////////////////////////////////////////////////////////////
// cache for animated bone data
struct CORELIB_EXPORT AnimationCache
{
	NvUInt32		m_numBones;
	NvChar*			m_pBoneNames;

	NvInt32			m_frameStart;
	NvInt32			m_frameEnd;
	NvUInt32		m_numFrames;

	atcore_float4x4*		m_pBoneMatrices;

	AnimationCache() :
		m_numBones(0),
		m_frameStart(0),
		m_frameEnd(0),
		m_numFrames(0),
		m_pBoneMatrices(nullptr),
		m_pBoneNames(nullptr)
		{
		}

	bool isValid() const {
		return (m_numBones > 0) && (m_numFrames > 0) && (m_pBoneMatrices != 0);
	}

	void Allocate();
	void Release();
	void Initialize(int numBones, NvInt32 frameStart, NvInt32 frameEnd);

	bool GetInterpolationInfo(float frameTime, int& indexStart, int& indexEnd, float &fracFrame);

	const atcore_float4x4* GetNodeMatrices(int index) const { return m_pBoneMatrices + index * m_numBones; }
	atcore_float4x4* GetNodeMatrices(int index) { return m_pBoneMatrices + index * m_numBones; }
	
	const char* GetBoneName(int index) const
	{
		return m_pBoneNames + index * NV_HAIR_MAX_STRING;
	}

	void	SetBoneName(int index, const char* boneName)
	{
		char* str = m_pBoneNames + index * NV_HAIR_MAX_STRING;
		strcpy_s(str, NV_HAIR_MAX_STRING, boneName);
	}

	int		FindBone(const char *boneName) const;
	bool	FindBoneMapping( int numBones, const NvChar* boneNames, int* mappedBoneId) const;

	~AnimationCache();
};

////////////////////////////////////////////////////////////////////////////////////////
// bone matrices at each frame
////////////////////////////////////////////////////////////////////////////////////////
struct CORELIB_EXPORT BoneData
{
	NvUInt32			m_numBones;
	NvChar*				m_pBoneNames;

	atcore_float4x4*			m_pPoseMatrices; // rest pose for each bone
	atcore_float4x4*			m_pBoneMatrices; // updated bone matrix
	atcore_float4x4*			m_pSkinMatrices; // interpolated skinning matrix for current frame
	atcore_dualquaternion*	m_pSkinDQs; // dual quats

	AnimationCache*			m_pAnimationCache; 
	int*					m_pMappedBoneId;
	int						m_nodeId;

public:
	BoneData() :
		m_numBones(0),
		m_pBoneNames(nullptr),
		m_pPoseMatrices(nullptr),
		m_pBoneMatrices(nullptr),
		m_pSkinMatrices(nullptr),
		m_pSkinDQs(nullptr),
		m_pAnimationCache(nullptr),
		m_pMappedBoneId(nullptr),
		m_nodeId(-1)
		{}

	void Allocate(NvUInt32 numBones);
	void Release();

	const char* getBoneName(int index)
	{
		return m_pBoneNames + index * NV_HAIR_MAX_STRING;
	}

	void setBoneName(int index, const char* boneName)
	{
		char* str = m_pBoneNames + index * NV_HAIR_MAX_STRING;
		strcpy_s(str, NV_HAIR_MAX_STRING, boneName);
	}

	void InitializeAnimationCache(AnimationCache* pGlobalCache, const char* nodeName);
	bool Update(float frameTime, const char* rootBoneName, bool bindPose, bool zup);

};

////////////////////////////////////////////////////////////////////////////////////////
// Skinning data
struct CORELIB_EXPORT SkinData
{
	BoneData			m_boneData;

	atcore_float4*		m_pBoneIndices;
	atcore_float4*		m_pBoneWeights;

public:
	SkinData() : 
		m_pBoneIndices(nullptr),
		m_pBoneWeights(nullptr)
		{}

	void Allocate(NvUInt32 numBones, NvUInt32 numVertices);
	void Release();
};


////////////////////////////////////////////////////////////////////////////////////////
struct CORELIB_EXPORT MeshDesc
{
	NvUInt32				m_NumVertices;
	NvUInt32				m_NumTriangles;

	atcore_float3*			m_pVertices;
	atcore_float3*			m_pVertexNormals;
	atcore_float3*			m_pFaceNormals;
	atcore_float3*			m_pTangents;
	atcore_float3			m_ColorRGB;

	NvUInt32*				m_pIndices;
	atcore_float2*			m_pTexCoords;

public:
	MeshDesc() :
		m_NumVertices(0),
		m_NumTriangles(0),

		m_pVertices(nullptr),
		m_pVertexNormals(nullptr),
		m_pFaceNormals(nullptr),
		m_pTangents(nullptr),

		m_pIndices(nullptr),
		m_pTexCoords(nullptr)
		{
			m_ColorRGB = gfsdk_makeFloat3(0, 0, 0);
		}

	void Allocate(NvUInt32 numVertices, NvUInt32 numTriangles);
	void Release();

	void UpdateNormal(bool updateVertexNormal = false);
};

////////////////////////////////////////////////////////////////////////////////////////
#define MATERIAL_NAME_SIZE	128

struct CORELIB_EXPORT MeshMaterial
{
	NvChar				m_name[MATERIAL_NAME_SIZE];

	atcore_float3		m_ambientColor;
	float				m_ambientFactor;

	atcore_float3		m_diffuseColor;
	float				m_diffuseFactor;

	float				m_specularFactor;
	float				m_shininess;

	NvChar				m_diffuseTexture[MATERIAL_NAME_SIZE];
	NvChar				m_specularTexture[MATERIAL_NAME_SIZE];
	NvChar				m_bumpTexture[MATERIAL_NAME_SIZE];
	NvChar				m_normalTexture[MATERIAL_NAME_SIZE];
	NvChar				m_transparentTexture[MATERIAL_NAME_SIZE];

public:
	MeshMaterial() 
		{}
};

