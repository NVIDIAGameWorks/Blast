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

#include "d3d12.h"
#include "d3dx12.h"

// GPU resources for texture 
struct D3D12TextureResource : public GPUShaderResource
{
	ID3D12Resource* m_pResource;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_Handle;
	int m_nIndexInHeap;

public:
	static GPUShaderResource* Create(
		ID3D12Resource* pResource, 
		CD3DX12_CPU_DESCRIPTOR_HANDLE handle,
		int nIndexInHeap) {
		D3D12TextureResource* pBuffer = new D3D12TextureResource; 
		pBuffer->m_pResource = pResource;
		pBuffer->m_Handle = handle;
		pBuffer->m_nIndexInHeap = nIndexInHeap;
		return pBuffer;
	}

	static ID3D12Resource* GetResource(GPUShaderResource* pBuffer)
	{
		D3D12TextureResource* pD3D12Buffer = dynamic_cast<D3D12TextureResource*>(pBuffer);
		if (!pD3D12Buffer)
			return 0;
		return pD3D12Buffer->m_pResource;
	}

	static CD3DX12_CPU_DESCRIPTOR_HANDLE GetHandle(GPUShaderResource* pBuffer)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE handle = {};
		D3D12TextureResource* pD3D12Buffer = dynamic_cast<D3D12TextureResource*>(pBuffer);
		if (pD3D12Buffer)
			handle = pD3D12Buffer->m_Handle;
		else
			handle.ptr = 0;
		return handle;
	}

	D3D12TextureResource()
	{
		m_pResource = 0;
	}

	void Release()
	{
		SAFE_RELEASE(m_pResource);
	}

	~D3D12TextureResource()
	{
		Release();
	}
};

