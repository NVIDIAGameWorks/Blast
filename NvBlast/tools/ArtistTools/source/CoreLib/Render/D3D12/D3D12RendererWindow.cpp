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
#include "D3D12RendererWindow.h"

#include "DXUT.h"
#include "DXUTgui.h"
#include "sdkmisc.h"

#include "D3D12RenderInterface.h"
#include "D3D12RenderContext.h"
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) { if (x) x->Release(); x = 0; }
#endif

///////////////////////////////////////////////////////////////////////////////
D3D12RenderWindow::D3D12RenderWindow()
{
	m_pRenderContext = D3D12RenderContext::Instance();

	m_pDialogResourceManager = 0;
	m_pTextHelper = 0;
}

///////////////////////////////////////////////////////////////////////////////
D3D12RenderWindow::~D3D12RenderWindow()
{
	Free();
}

///////////////////////////////////////////////////////////////////////////////
bool D3D12RenderWindow::Create( HWND hWnd, unsigned int nSamples )
{
	m_pRenderContext->SetSampleCount(nSamples);

	RECT rc;
	GetClientRect((HWND)hWnd, &rc);
	int wBuf = rc.right - rc.left;
	int hBuf = rc.bottom- rc.top;

	ID3D11Device *pDevice = m_pRenderContext->GetDevice11();
	ID3D11DeviceContext* pDeviceContext = m_pRenderContext->GetDeviceContext();
	if (nullptr != pDevice && nullptr != pDeviceContext)
	{
		m_pDialogResourceManager = new CDXUTDialogResourceManager;
		m_pDialogResourceManager->OnD3D11CreateDevice(pDevice, pDeviceContext);
		m_pTextHelper = new CDXUTTextHelper(pDevice, pDeviceContext, m_pDialogResourceManager, 15);
	}

	m_pRenderContext->InitSwapchain(wBuf, hBuf, hWnd);
	Resize(wBuf, hBuf);

	return true;
}

///////////////////////////////////////////////////////////////////////////////
void D3D12RenderWindow::Free()
{
	FreeBuffer();

	SAFE_DELETE(m_pTextHelper);

	if (m_pDialogResourceManager)
	{
		m_pDialogResourceManager->OnD3D11DestroyDevice();
		SAFE_DELETE(m_pDialogResourceManager);
	}
}

///////////////////////////////////////////////////////////////////////////////
void D3D12RenderWindow::FreeBuffer()
{
}

///////////////////////////////////////////////////////////////////////////////
void D3D12RenderWindow::Present()
{
//	m_pRenderContext->ReleaseRenderTarget();

//	m_pRenderContext->PostRender();

	m_pRenderContext->Present();
}

///////////////////////////////////////////////////////////////////////////////
void D3D12RenderWindow::Clear(float r, float g, float b)
{
	m_pRenderContext->PreRender();

	m_pRenderContext->SetClearColor(0, r, g, b);
	m_pRenderContext->AcquireRenderTarget(true);
}

///////////////////////////////////////////////////////////////////////////////
bool D3D12RenderWindow::Resize( int w, int h )
{
	assert(w > 0 && h > 0);

	m_pRenderContext->ResizeSwapchain(w, h);

	if (m_pDialogResourceManager)
	{
		ID3D11Device *pDevice = m_pRenderContext->GetDevice11();
		D3D12_RESOURCE_DESC descTex2D = m_pRenderContext->GetBackBufferDesc();

		DXGI_SURFACE_DESC backbufferDesc;
		backbufferDesc.Width = descTex2D.Width;
		backbufferDesc.Height = descTex2D.Height;
		backbufferDesc.Format = descTex2D.Format;
		backbufferDesc.SampleDesc = descTex2D.SampleDesc;

		m_pDialogResourceManager->OnD3D11ResizedSwapChain(pDevice, &backbufferDesc);
	}

	return true;
}

