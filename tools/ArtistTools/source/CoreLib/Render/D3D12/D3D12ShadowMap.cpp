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
#include "D3D12ShadowMap.h"

#include "RenderResources.h"

#include "D3D12RenderInterface.h"
#include "D3D12TextureResource.h"
#include "D3D12RenderContext.h"

//////////////////////////////////////////////////////////////////////////////
D3D12ShadowMap::D3D12ShadowMap(int resolution)
{
	D3D12RenderContext* pContext = D3D12RenderContext::Instance();
	m_nIndex = pContext->AllocRenderTargetIndex();
	m_pRenderTarget = pContext->CreateRenderTarget(m_nIndex, resolution, resolution);
	ID3D12Resource* pTexture = pContext->GetTexture(m_nIndex);
	int nIndexInHeap = -1;
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle = pContext->NVHairINT_CreateD3D12Texture(pTexture, nIndexInHeap);
	m_ShadowResource.m_pResource = pTexture;
	m_ShadowResource.m_Handle = handle;
	m_ShadowResource.m_nIndexInHeap = nIndexInHeap;
}

//////////////////////////////////////////////////////////////////////////////
D3D12ShadowMap::~D3D12ShadowMap()
{
	D3D12RenderContext* pContext = D3D12RenderContext::Instance();
	pContext->NVHairINT_DestroyD3D12Texture(m_ShadowResource.m_nIndexInHeap);

	Release();
}

//////////////////////////////////////////////////////////////////////////////
void D3D12ShadowMap::Release()
{
}

//////////////////////////////////////////////////////////////////////////////
GPUShaderResource* D3D12ShadowMap::GetShadowSRV()
{
	return &m_ShadowResource;
}

//////////////////////////////////////////////////////////////////////////////
bool D3D12ShadowMap::isValid()
{
	return m_pRenderTarget != nullptr;
}

//////////////////////////////////////////////////////////////////////////////
void D3D12ShadowMap::BeginRendering(float clearDepth)
{
	if (!isValid())
		return;

	D3D12RenderContext* pContext = D3D12RenderContext::Instance();
	pContext->ReleaseRenderTarget();

	pContext->AcquireRenderTarget(true, m_nIndex);
}

//////////////////////////////////////////////////////////////////////////////
void D3D12ShadowMap::EndRendering()
{
	if (!isValid())
		return;

	D3D12RenderContext* pContext = D3D12RenderContext::Instance();
	pContext->ReleaseRenderTarget(m_nIndex);

	pContext->AcquireRenderTarget();
}

