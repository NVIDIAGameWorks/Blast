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
#include "MeshData.h"

#include "AnimUtil.h"
#include "RenderResources.h"

///////////////////////////////////////////////////////////////////////////////
void CreateCPUResource(MeshData* pMeshData, const MeshDesc &meshDesc)
{
	if (!pMeshData)
		return;

	pMeshData->m_NumVertices = meshDesc.m_NumVertices;
	pMeshData->m_NumIndices = meshDesc.m_NumTriangles * 3;

	int numIndices = pMeshData->m_NumIndices;

	pMeshData->m_pIndices = new NvUInt32[numIndices];
	pMeshData->m_pMeshVertices = new MeshData::MeshVertex[numIndices];

	NvUInt32* pVertUsed = new NvUInt32[meshDesc.m_NumVertices];
	memset(pVertUsed, meshDesc.m_NumVertices, sizeof(NvUInt32) * meshDesc.m_NumVertices);
	for (NvUInt32 i = 0; i < numIndices; i++)
	{
		NvUInt32 vidx = pMeshData->m_pIndices[i] = meshDesc.m_pIndices[i];

		MeshData::MeshVertex&v = pMeshData->m_pMeshVertices[i];

		v.pos			= meshDesc.m_pVertices[vidx];
		v.vertexNormal	= meshDesc.m_pVertexNormals[i];
		v.faceNormal	= meshDesc.m_pFaceNormals[i];
		v.tangent		= meshDesc.m_pTangents[i];
		v.texcoord		= meshDesc.m_pTexCoords[i];
		v.vertexId		= (float)vidx;
		pVertUsed[vidx]	= i;
	}
	int numUniqueIndices = 0;
	for (NvUInt32 i = 0; i < meshDesc.m_NumVertices; i++)
	{
		if (meshDesc.m_NumVertices != pVertUsed[i])
			++numUniqueIndices;
	}
	pMeshData->m_NumUniqueIndices = numUniqueIndices;
	pMeshData->m_pUniqueIndices = new NvUInt32[numUniqueIndices];
	for (NvUInt32 i = 0, idx = 0; i < meshDesc.m_NumVertices; i++)
	{
		if (meshDesc.m_NumVertices != pVertUsed[i])
		{
			pMeshData->m_pUniqueIndices[idx++] = pVertUsed[i];
		}
	}
	delete[] pVertUsed;
}

///////////////////////////////////////////////////////////////////////////////
MeshData* MeshData::Create(MeshDesc &meshDesc, SkinData& skinData)
{
	MeshData* pMeshData = new MeshData;

	CreateCPUResource(pMeshData, meshDesc);

	pMeshData->m_GPUMeshResources = GPUMeshResources::Create(pMeshData, skinData);

	return pMeshData;
}

///////////////////////////////////////////////////////////////////////////////
void MeshData::Release()
{
	if (m_pMeshVertices) 
		delete [] m_pMeshVertices;

	if (m_pIndices) 
		delete [] m_pIndices;

	if (m_pUniqueIndices)
		delete[] m_pUniqueIndices;

	m_GPUMeshResources->Release();
	delete m_GPUMeshResources;
	m_GPUMeshResources = NULL;
}
