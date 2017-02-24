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
#include "D3D11ShadowMap.h"

#include "RenderResources.h"

#include "D3D11RenderInterface.h"
#include "D3D11TextureResource.h"

namespace
{
//////////////////////////////////////////////////////////////////////////////////////////////////
D3D11_TEXTURE2D_DESC CreateD3D11TextureDesc(
	DXGI_FORMAT Format, UINT Width, UINT Height,
	UINT BindFlags, UINT SampleCount = 1, D3D11_USAGE Usage = D3D11_USAGE_DEFAULT, UINT CPUAccessFlags = 0,
	UINT MiscFlags = 0, UINT ArraySize = 1, UINT MipLevels = 1)
{

	D3D11_TEXTURE2D_DESC desc;
	
	desc.Format                 = Format;
	desc.Width					= Width;
	desc.Height					= Height;

	desc.ArraySize				= ArraySize;
	desc.MiscFlags				= MiscFlags;
	desc.MipLevels				= MipLevels;

	desc.SampleDesc.Count		= SampleCount;
	desc.SampleDesc.Quality		= 0;
	desc.BindFlags				= BindFlags;
	desc.Usage					= Usage;
	desc.CPUAccessFlags			= CPUAccessFlags;

	return desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
D3D11_DEPTH_STENCIL_VIEW_DESC CreateD3D11DSVDesc(
	DXGI_FORMAT Format, D3D11_DSV_DIMENSION ViewDimension,  
	UINT Flags = 0, UINT MipSlice = 0)
{
	D3D11_DEPTH_STENCIL_VIEW_DESC desc;
	
	desc.Format                    = Format;
	desc.ViewDimension             = ViewDimension;
	desc.Flags					   = Flags;
	desc.Texture2D.MipSlice        = MipSlice;

	return desc;
}

}

//////////////////////////////////////////////////////////////////////////////
D3D11ShadowMap::D3D11ShadowMap(int resolution)
{
	m_pShadowTexture = nullptr;
	m_pShadowRTV = nullptr;
	m_pShadowSRV = nullptr;

	m_pDepthTexture = nullptr;
	m_pDepthDSV = nullptr;

	ID3D11Device* pd3dDevice = RenderInterfaceD3D11::GetDevice();
	if (!pd3dDevice)
		return;

	{
		m_viewport.Width  = float(resolution);
		m_viewport.Height = float(resolution);
		m_viewport.MinDepth = 0;
		m_viewport.MaxDepth = 1;
		m_viewport.TopLeftX = 0;
		m_viewport.TopLeftY = 0;
	}

	HRESULT hr;

	// create shadow render target
	{
		D3D11_TEXTURE2D_DESC texDesc = CreateD3D11TextureDesc(
			DXGI_FORMAT_R32_FLOAT, UINT(m_viewport.Width), UINT(m_viewport.Height),
			D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
	
		hr = pd3dDevice->CreateTexture2D( &texDesc, NULL, &m_pShadowTexture );
		hr = pd3dDevice->CreateShaderResourceView( m_pShadowTexture, NULL, &m_pShadowSRV );
		hr = pd3dDevice->CreateRenderTargetView(m_pShadowTexture, NULL, &m_pShadowRTV);

		m_shadowResource.m_pD3D11Resource = m_pShadowSRV;
	}

	// create shadow depth stencil
	{
		D3D11_TEXTURE2D_DESC texDesc = CreateD3D11TextureDesc(
			DXGI_FORMAT_R32_TYPELESS, UINT(m_viewport.Width), UINT(m_viewport.Height),
			D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE); 

		hr = pd3dDevice->CreateTexture2D( &texDesc, NULL, &m_pDepthTexture );

		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = CreateD3D11DSVDesc(
			DXGI_FORMAT_D32_FLOAT, D3D11_DSV_DIMENSION_TEXTURE2D);

		hr = pd3dDevice->CreateDepthStencilView(m_pDepthTexture, &dsvDesc, &m_pDepthDSV);		
	}
}

//////////////////////////////////////////////////////////////////////////////
D3D11ShadowMap::~D3D11ShadowMap()
{
	Release();
}

//////////////////////////////////////////////////////////////////////////////
void D3D11ShadowMap::Release()
{
	SAFE_RELEASE(m_pShadowTexture);
	SAFE_RELEASE(m_pShadowRTV);
	SAFE_RELEASE(m_pShadowSRV);
	SAFE_RELEASE(m_pDepthTexture);
	SAFE_RELEASE(m_pDepthDSV);

	m_shadowResource.m_pD3D11Resource = 0;
}

//////////////////////////////////////////////////////////////////////////////
GPUShaderResource* D3D11ShadowMap::GetShadowSRV()
{
	return &m_shadowResource;
}

//////////////////////////////////////////////////////////////////////////////
bool D3D11ShadowMap::isValid()
{
	return m_pShadowTexture && m_pShadowRTV && m_pShadowSRV && m_pDepthTexture && m_pDepthDSV;
}

//////////////////////////////////////////////////////////////////////////////
void D3D11ShadowMap::BeginRendering(float clearDepth)
{
	if (!isValid())
		return;

	ID3D11DeviceContext* pd3dContext = RenderInterfaceD3D11::GetDeviceContext();
	if (!pd3dContext)
		return;

	pd3dContext->OMGetRenderTargets(1, &m_pPreviousRTV, &m_pPreviousDSV);
	m_numPreviousViewports = 1;
	pd3dContext->RSGetViewports(&m_numPreviousViewports, m_previousViewport);

	pd3dContext->OMSetRenderTargets( 1, &m_pShadowRTV, m_pDepthDSV);
	pd3dContext->RSSetViewports(1, &m_viewport);

	float ClearColor[4] = { clearDepth, clearDepth, clearDepth, clearDepth};

	pd3dContext->ClearRenderTargetView( m_pShadowRTV, ClearColor );		
	pd3dContext->ClearDepthStencilView( m_pDepthDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );
}

//////////////////////////////////////////////////////////////////////////////
void D3D11ShadowMap::EndRendering()
{
	if (!isValid())
		return;

	ID3D11DeviceContext* pd3dContext = RenderInterfaceD3D11::GetDeviceContext();
	if (!pd3dContext)
		return;

	pd3dContext->OMSetRenderTargets(0, NULL, NULL);

	if (m_pPreviousRTV)
	{
		pd3dContext->OMSetRenderTargets(1, &m_pPreviousRTV, m_pPreviousDSV);
		m_pPreviousRTV->Release();
		if (m_pPreviousDSV) m_pPreviousDSV->Release();
	}

	if (m_numPreviousViewports)
		pd3dContext->RSSetViewports(m_numPreviousViewports, m_previousViewport);
}

