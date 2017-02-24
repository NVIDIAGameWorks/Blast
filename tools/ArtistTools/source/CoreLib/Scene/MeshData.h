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

#include "RenderResources.h"

class MeshDesc;
class SkinData;

//////////////////////////////////////////////////////////////////////////////////////
// Helper for rendering graphics mesh
//////////////////////////////////////////////////////////////////////////////////////
class CORELIB_EXPORT MeshData
{
public:

	struct MeshVertex
	{
		atcore_float3  pos;
		atcore_float3  vertexNormal;
		atcore_float3  faceNormal;
		atcore_float3  tangent;
		atcore_float2  texcoord;
		float		  vertexId;
	};

public:
	static MeshData* Create(MeshDesc &meshDesc, SkinData& skinData);
	void Release();

public:
	int					m_NumVertices;
	int					m_NumIndices;

	MeshVertex*			m_pMeshVertices;
	NvUInt32*			m_pIndices;

	// m_pUniqueIndices and m_NumUniqueIndices are used to boost bounding calculation in FurMesh.cpp
	NvUInt32*			m_pUniqueIndices;
	NvUInt32			m_NumUniqueIndices;

	// gpu resources
	GPUMeshResources*	m_GPUMeshResources;
};


