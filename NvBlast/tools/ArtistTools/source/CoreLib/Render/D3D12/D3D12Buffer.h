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

#include "RenderResources.h"

#include <d3d12.h>

// GPU resources for texture 
struct GPUBufferD3D12 : public GPUBufferResource
{
	ID3D12Resource*	m_pBuffer;
	UINT m_pBufferSize;

	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

public:
	static GPUBufferResource* Create(ID3D12Resource* pResource, int nSize) {
		GPUBufferD3D12* pBuffer = new GPUBufferD3D12; 
		pBuffer->m_pBuffer = pResource;
		pBuffer->m_pBufferSize = nSize;
		return pBuffer;
	}

	static ID3D12Resource* GetResource(GPUBufferResource* pBuffer)
	{
		GPUBufferD3D12* pD3D12Buffer = dynamic_cast<GPUBufferD3D12*>(pBuffer);
		if (!pD3D12Buffer)
			return 0;
		return pD3D12Buffer->m_pBuffer;
	}

	static D3D12_VERTEX_BUFFER_VIEW* GetVertexView(GPUBufferResource* pBuffer)
	{
		GPUBufferD3D12* pD3D12Buffer = dynamic_cast<GPUBufferD3D12*>(pBuffer);
		if (!pD3D12Buffer)
			return 0;

		pD3D12Buffer->m_vertexBufferView.BufferLocation = pD3D12Buffer->m_pBuffer->GetGPUVirtualAddress();
		pD3D12Buffer->m_vertexBufferView.StrideInBytes = 0;
		pD3D12Buffer->m_vertexBufferView.SizeInBytes = pD3D12Buffer->m_pBufferSize;
		return &pD3D12Buffer->m_vertexBufferView;
	}

	~GPUBufferD3D12()
	{
		Release();
	}

	void Release()
	{
		SAFE_RELEASE(m_pBuffer);
	}
};

