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
#include "D3D11RendererWindow.h"

#include "windows.h"

#include "DXUT.h"
#include "DXUTgui.h"
#include "sdkmisc.h"

#include "D3D11RenderInterface.h"
#include "Nv.h"
#ifdef USE_11ON12_WRAPPER
#include "D3D11on12Wrapper.h"
#endif // USE_11ON12_WRAPPER
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) { if (x) x->Release(); x = 0; }
#endif

///////////////////////////////////////////////////////////////////////////////
D3D11RenderWindow::D3D11RenderWindow() :
	 m_sampleCount(8)
	, m_sampleQuality(0)
#ifdef USE_11ON12_WRAPPER
#else
	, m_pDXGISwapChain(NV_NULL)
	, m_pD3D11BackBuffer(NV_NULL)
	, m_pD3D11DepthBuffer(NV_NULL)
	, m_pD3D11RenderTargetView(NV_NULL)
	, m_pD3D11DepthStencilView(NV_NULL)
#endif // USE_11ON12_WRAPPER
{
	
}

///////////////////////////////////////////////////////////////////////////////
D3D11RenderWindow::~D3D11RenderWindow()
{
	Free();
}

///////////////////////////////////////////////////////////////////////////////
bool D3D11RenderWindow::Create( HWND hWnd, unsigned int nSamples )
{
#ifdef USE_11ON12_WRAPPER
#else
	if (m_pD3D11BackBuffer != NV_NULL || m_pD3D11RenderTargetView != NV_NULL)
		Free();
#endif // USE_11ON12_WRAPPER

	ID3D11Device *pDevice = RenderInterfaceD3D11::GetDevice();
	if (!pDevice)
		return false;

	ID3D11DeviceContext* pDeviceContext = RenderInterfaceD3D11::GetDeviceContext();
	if (!pDeviceContext)
		return false;

	if(nSamples > D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT)
		nSamples = D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT;

	m_sampleCount = nSamples;
	m_sampleQuality = 0;
	if(nSamples > 1)
	{
		pDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, nSamples, &m_sampleQuality);
		assert(m_sampleQuality > 0);
		m_sampleQuality--;
	}

	RECT rc;
	GetClientRect((HWND)hWnd, &rc);
	int wBuf = rc.right - rc.left;
	int hBuf = rc.bottom- rc.top;

#ifdef USE_11ON12_WRAPPER
	D3D11on12Wrapper::InitSwapchain(wBuf, hBuf, hWnd);
#else
	DXGI_SWAP_CHAIN_DESC	swapChainDesc;
	{
		memset(&swapChainDesc, 0, sizeof(DXGI_SWAP_CHAIN_DESC));
		swapChainDesc.BufferDesc.Width = wBuf;
		swapChainDesc.BufferDesc.Height= hBuf;
		swapChainDesc.BufferDesc.Format= DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	
		swapChainDesc.BufferCount = 1;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		swapChainDesc.OutputWindow = (HWND)hWnd;
		swapChainDesc.SampleDesc.Count = m_sampleCount;
		swapChainDesc.SampleDesc.Quality = m_sampleQuality;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Windowed = true;
	}

	// Create the swap chain
	IDXGIFactory1* pDXGIFactory = RenderInterfaceD3D11::GetDXGIFactory();
	HRESULT res = pDXGIFactory->CreateSwapChain(pDevice, &swapChainDesc, &m_pDXGISwapChain);
	if (FAILED(res)) return false;
#endif // USE_11ON12_WRAPPER

	// create DXUT text rendering class
	m_pDialogResourceManager = new CDXUTDialogResourceManager;
	m_pDialogResourceManager->OnD3D11CreateDevice(pDevice, pDeviceContext);
	m_pTextHelper = new CDXUTTextHelper( pDevice, pDeviceContext, m_pDialogResourceManager, 15 );

	CreateRenderTarget();
	Resize(wBuf, hBuf);
	return true;// SUCCEEDED(res);
}

///////////////////////////////////////////////////////////////////////////////
void D3D11RenderWindow::Free()
{
	FreeBuffer();

	SAFE_DELETE(m_pTextHelper);

#ifdef USE_11ON12_WRAPPER
#else
	SAFE_RELEASE(m_pDXGISwapChain);
#endif // USE_11ON12_WRAPPER

	if (m_pDialogResourceManager)
	{
		m_pDialogResourceManager->OnD3D11DestroyDevice();
		SAFE_DELETE(m_pDialogResourceManager);
	}
}

///////////////////////////////////////////////////////////////////////////////
void D3D11RenderWindow::FreeBuffer()
{
#ifdef USE_11ON12_WRAPPER
#else
	SAFE_RELEASE(m_pD3D11RenderTargetView);
	SAFE_RELEASE(m_pD3D11BackBuffer);
	SAFE_RELEASE(m_pD3D11DepthStencilView);
	SAFE_RELEASE(m_pD3D11DepthBuffer);
#endif // USE_11ON12_WRAPPER
}

///////////////////////////////////////////////////////////////////////////////
void D3D11RenderWindow::Present()
{
#ifdef USE_11ON12_WRAPPER
	D3D11on12Wrapper::EndScene();
#else
	assert(m_pDXGISwapChain);

	if(m_pDXGISwapChain)
	{
		//m_pDXGISwapChain->Present(1, 0);	// present in vsync
		m_pDXGISwapChain->Present(0, 0);	// present in vsync off
	}
#endif // USE_11ON12_WRAPPER
}

///////////////////////////////////////////////////////////////////////////////
void D3D11RenderWindow::Clear(float r, float g, float b)
{
#ifdef USE_11ON12_WRAPPER
	D3D11on12Wrapper::BeginScene();
	D3D11on12Wrapper::ClearScene(r, g, b);
#else
	const float clearColor[4] = {r, g, b, 1.0f}; 

	ID3D11DeviceContext* pD3DContext = RenderInterfaceD3D11::GetDeviceContext();
	pD3DContext->ClearRenderTargetView(m_pD3D11RenderTargetView, clearColor);
	pD3DContext->ClearDepthStencilView(m_pD3D11DepthStencilView, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);
#endif // USE_11ON12_WRAPPER
}

///////////////////////////////////////////////////////////////////////////////
bool D3D11RenderWindow::Resize(int w, int h)
{
	assert(w > 0 && h > 0);
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	HRESULT res0 = m_pDXGISwapChain->GetDesc(&swapChainDesc);
	if (w == (LONG)swapChainDesc.BufferDesc.Width &&
		h == (LONG)swapChainDesc.BufferDesc.Height)
		return true;

#ifdef USE_11ON12_WRAPPER
#else
	//ID3D11DeviceContext* pD3DContext = RenderInterfaceD3D11::GetDeviceContext();
	//ID3D11RenderTargetView *nullRTV = NULL;
	//pD3DContext->OMSetRenderTargets(1, &nullRTV, NULL);
	FreeBuffer();
	swapChainDesc.BufferDesc.Width = w;
	swapChainDesc.BufferDesc.Height = h;
	//HRESULT res = m_pDXGISwapChain->ResizeBuffers(1, w, h, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	HRESULT res = m_pDXGISwapChain->ResizeBuffers(swapChainDesc.BufferCount, swapChainDesc.BufferDesc.Width,
		swapChainDesc.BufferDesc.Height, swapChainDesc.BufferDesc.Format,
		swapChainDesc.Flags);
	assert(SUCCEEDED(res));
#endif
	return CreateRenderTarget();
}

bool D3D11RenderWindow::CreateRenderTarget()
{
#ifdef USE_11ON12_WRAPPER
	D3D11on12Wrapper::ResizeScene(w, h);
#else
	ID3D11Device* pDevice = RenderInterfaceD3D11::GetDevice();
	// Retrieve the 2D back buffer 
	HRESULT res = m_pDXGISwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&m_pD3D11BackBuffer);
	assert(SUCCEEDED(res));

	D3D11_TEXTURE2D_DESC descTex2D;
	m_pD3D11BackBuffer->GetDesc(&descTex2D);

	// Create the render target view
	D3D11_RENDER_TARGET_VIEW_DESC descRenderTargetView;
	{
		memset(&descRenderTargetView, 0, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
		descRenderTargetView.Format				= descTex2D.Format;
		descRenderTargetView.ViewDimension		= D3D11_RTV_DIMENSION_TEXTURE2DMS;	// -MS dimension to support MSAA
		descRenderTargetView.Texture2D.MipSlice	= 0;
	}

	res = pDevice->CreateRenderTargetView(m_pD3D11BackBuffer, &descRenderTargetView, &m_pD3D11RenderTargetView);

	assert(SUCCEEDED(res));

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	HRESULT res0 = m_pDXGISwapChain->GetDesc(&swapChainDesc);
	// Create the depth/stencil buffer and view
	D3D11_TEXTURE2D_DESC depthStencilDesc;

	depthStencilDesc.Width     = swapChainDesc.BufferDesc.Width;
	depthStencilDesc.Height    = swapChainDesc.BufferDesc.Height;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format    = DXGI_FORMAT_R24G8_TYPELESS;
	depthStencilDesc.SampleDesc.Count   = m_sampleCount;
	depthStencilDesc.SampleDesc.Quality = m_sampleQuality;
	depthStencilDesc.Usage          = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags      = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depthStencilDesc.CPUAccessFlags = 0; 
	depthStencilDesc.MiscFlags      = 0;

	res = pDevice->CreateTexture2D(&depthStencilDesc, 0, &m_pD3D11DepthBuffer);
	assert(SUCCEEDED(res));

	D3D11_DEPTH_STENCIL_VIEW_DESC descDepthStencilView;
	memset(&descDepthStencilView, 0, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
	descDepthStencilView.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepthStencilView.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;	// -MS dimension to support MSAA
	descDepthStencilView.Texture2D.MipSlice = 0;

	res = pDevice->CreateDepthStencilView(m_pD3D11DepthBuffer, &descDepthStencilView, &m_pD3D11DepthStencilView);
	assert(SUCCEEDED(res));

	m_Width = swapChainDesc.BufferDesc.Width;
	m_Height= swapChainDesc.BufferDesc.Height;

	/////////////////////////////////////////
	if (m_pDialogResourceManager)
	{
		DXGI_SURFACE_DESC backbufferDesc;
		backbufferDesc.Width = descTex2D.Width;
		backbufferDesc.Height = descTex2D.Height;
		backbufferDesc.Format = descTex2D.Format;
		backbufferDesc.SampleDesc = descTex2D.SampleDesc;

		m_pDialogResourceManager->OnD3D11ResizedSwapChain(pDevice, &backbufferDesc);
	}

	// assume always the current render window, bind to render context immediately
	ID3D11DeviceContext* pD3DContext = RenderInterfaceD3D11::GetDeviceContext();
	pD3DContext->OMSetRenderTargets(1, &m_pD3D11RenderTargetView, m_pD3D11DepthStencilView);

	// set the viewport transform
	D3D11_VIEWPORT vp;
	{
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		vp.Width    = (float)m_Width;
		vp.Height   = (float)m_Height;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
	}
	pD3DContext->RSSetViewports(1, &vp);
#endif // USE_11ON12_WRAPPER

	return true;
}

