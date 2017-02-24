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
#include "D3D11Util.h"

#include "D3D11Shaders.h"
#include "D3D11RenderShader.h"

#include "D3DX10tex.h"
#include "D3DX11tex.h"

#include "D3D11Wrapper.h"
#include "DXUT.h"
#include "DXUTgui.h"
#include "sdkmisc.h"
#include "D3D11RendererWindow.h"
#include "SimpleRenderable.h"
//#include "MeshShaderParam.h"
#include "D3D11RenderInterface.h"
#include "D3D11TextureResource.h"

namespace D3D11Util
{
	using namespace RenderInterface;

	// D3D hook to render window
	D3D11RenderWindow*		g_pRenderWindow = 0;

///////////////////////////////////////////////////////////////////////////////
#include "../../../../../../external/stb_image/stb_image.c"

GPUShaderResource* 
CreateTextureSRV(const char* texturename)
{
	ID3D11Device* pDevice = RenderInterfaceD3D11::GetDevice();
	if (!pDevice)
		return 0;

	D3DX11_IMAGE_LOAD_INFO texLoadInfo;
	
	texLoadInfo.MipLevels = 8;
	texLoadInfo.MipFilter = D3DX10_FILTER_TRIANGLE;
	texLoadInfo.Filter = D3DX10_FILTER_TRIANGLE;

	ID3D11Resource *pRes = 0;

	HRESULT hr;
	D3DX11CreateTextureFromFileA(pDevice, texturename, &texLoadInfo, NULL, &pRes, &hr);

	ID3D11Texture2D* texture = NULL;

	if (!pRes)
	{
		// Try stb_image for .TGA
		int width = 0;
		int height = 0;
		int numComponents = 0;
		unsigned char *pSTBIRes = stbi_load(texturename, &width, &height, &numComponents, 4);

		if (!pSTBIRes)
			return 0;

		const int requestedMipLevels = texLoadInfo.MipLevels;

		D3D11_SUBRESOURCE_DATA* initData = new D3D11_SUBRESOURCE_DATA[requestedMipLevels];
		ZeroMemory(initData, sizeof(D3D11_SUBRESOURCE_DATA)*requestedMipLevels);

		struct Pixel
		{
			unsigned char rgba[4];
		};

		// prepare target buffer just large enough to include all the mip levels
		Pixel* targets = new Pixel[width*height*2];
		
		// copy the first mip level
		memcpy(targets, pSTBIRes, width*height*4);
		
		// now it's OK to delete the original
		if (pSTBIRes)
			stbi_image_free(pSTBIRes);

		// current mip level width and height
		int mipWidth = width;
		int mipHeight = height;
		
		// actual mip levels
		int mipLevels = 0;

		// current data
		Pixel* source = targets;
		Pixel* target = nullptr;

		for (int idx = 0; idx < requestedMipLevels; ++idx)
		{
			// set initData
			initData[idx].pSysMem = source;
			initData[idx].SysMemPitch = mipWidth*4;
			mipLevels++;

			// skip generating mip for 1x1
			if ((mipWidth == 1) && (mipHeight == 1))
				break;

			// skip generating mip for the last level
			if (idx == (requestedMipLevels-1))
				break;

			// buffer for the next mip level
			target = &source[mipWidth*mipHeight];

			const int prevWidth = mipWidth; // previous mip's width

			// generate the next mip level
			mipWidth = max(1, mipWidth >> 1);
			mipHeight = max(1, mipHeight >> 1);

			Pixel samples[4];

			for (int y = 0; y < mipHeight; ++y)
			{
				for (int x = 0; x < mipWidth; ++x)
				{
					const int px = x*2; // x in previous mip
					const int py = y*2; // y in previous mip

					samples[0] = source[py*prevWidth + px]; // left top
					samples[1] = source[py*prevWidth + px+1]; // right top
					samples[2] = source[(py+1)*prevWidth + px]; // left bottom
					samples[3] = source[(py+1)*prevWidth + px+1]; // right bottom

					// for each component
					for (int comp = 0; comp < 4; ++comp)
					{
						// do the linear box filter for lower mip level
						target[y*mipWidth + x].rgba[comp] = (samples[0].rgba[comp] + samples[1].rgba[comp] + samples[2].rgba[comp] + samples[3].rgba[comp])/4;
					}
				}
			}

			// update source
			source = target;
		}

		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
		desc.ArraySize = 1;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.Height = height;
		desc.Width = width;
		desc.MipLevels = mipLevels;
		desc.MiscFlags = 0;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;

		HRESULT ret = pDevice->CreateTexture2D(&desc, initData, &texture);
	
		delete initData;
		delete targets;
		
		if (ret != S_OK)
			return 0;
	}
	else
	{
		pRes->QueryInterface(__uuidof( ID3D11Texture2D ), (LPVOID*)&texture);
	}

	D3D11_TEXTURE2D_DESC desc;
	texture->GetDesc( &desc );

	ID3D11ShaderResourceView* pTextureSRV = 0;

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	ZeroMemory( &SRVDesc, sizeof(SRVDesc) );
	SRVDesc.Format = desc.Format;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.MipLevels = desc.MipLevels;
	pDevice->CreateShaderResourceView(texture, &SRVDesc, &pTextureSRV);

	if (texture) texture->Release();

	if (pRes)
		pRes->Release();

	return D3D11TextureResource::Create(pTextureSRV);
}


///////////////////////////////////////////////////////////////////////////////
bool GetDeviceInfoString(wchar_t *str)
{
	IDXGIAdapter* pAdapter = RenderInterfaceD3D11::GetAdapter();
	if (!pAdapter)
		return false;

	auto adapterDescription = DXGI_ADAPTER_DESC();
	pAdapter->GetDesc(&adapterDescription);

	WCHAR* pDescStr = adapterDescription.Description;
	float memInGB = float(adapterDescription.DedicatedVideoMemory) / 1e9f;
	swprintf_s(str, 1000, L"%s(%.1fGb)\n", pDescStr, memInGB);

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Render window interafce
/////////////////////////////////////////////////////////////////////////////////////////
bool CreateRenderWindow(HWND hWnd, int nSamples)
{
	SAFE_DELETE(g_pRenderWindow);

	g_pRenderWindow = new D3D11RenderWindow;
	return g_pRenderWindow->Create(hWnd, nSamples);
}

D3DHandles& GetDeviceHandles(D3DHandles& deviceHandles)
{
	deviceHandles.pAdapter = RenderInterfaceD3D11::GetAdapter();
	deviceHandles.pFactory = RenderInterfaceD3D11::GetDXGIFactory();
	deviceHandles.pDevice = RenderInterfaceD3D11::GetDevice();
	deviceHandles.pDeviceContext = RenderInterfaceD3D11::GetDeviceContext();

	deviceHandles.pDXGISwapChain = g_pRenderWindow->m_pDXGISwapChain;
	deviceHandles.pD3D11BackBuffer = g_pRenderWindow->m_pD3D11BackBuffer;
	deviceHandles.pD3D11RenderTargetView = g_pRenderWindow->m_pD3D11RenderTargetView;
	deviceHandles.pD3D11DepthBuffer = g_pRenderWindow->m_pD3D11DepthBuffer;
	deviceHandles.pD3D11DepthStencilView = g_pRenderWindow->m_pD3D11DepthStencilView;
	return deviceHandles;
}

void DestroyRenderWindow()
{
	SAFE_DELETE(g_pRenderWindow);
}

bool ResizeRenderWindow(int w, int h)
{
	if (!g_pRenderWindow)
		return false;

	return g_pRenderWindow->Resize(w,h);
}

void PresentRenderWindow()
{
	if (!g_pRenderWindow)
		return;
	
	g_pRenderWindow->Present();
}

void ClearRenderWindow(float r, float g, float b)
{
	if (!g_pRenderWindow)
		return;
	
	g_pRenderWindow->Clear(r,g,b);
}

/////////////////////////////////////////////////////////////////////////////////////////
// Text draw helper functions (using DXUT)
/////////////////////////////////////////////////////////////////////////////////////////
void TxtHelperBegin()
{
	if (!g_pRenderWindow || !g_pRenderWindow->m_pTextHelper)
		return;
		
	g_pRenderWindow->m_pTextHelper->Begin();
}

void TxtHelperEnd()
{
	if (!g_pRenderWindow || !g_pRenderWindow->m_pTextHelper)
		return;

	g_pRenderWindow->m_pTextHelper->End();
}

void TxtHelperSetInsertionPos(int x, int y)
{
	if (!g_pRenderWindow || !g_pRenderWindow->m_pTextHelper)
		return;

	g_pRenderWindow->m_pTextHelper->SetInsertionPos(x, y);
}

void TxtHelperSetForegroundColor(float r, float g, float b, float a)
{
	if (!g_pRenderWindow || !g_pRenderWindow->m_pTextHelper)
		return;

	g_pRenderWindow->m_pTextHelper->SetForegroundColor(DirectX::XMFLOAT4(r,g,b,a));
}

void TxtHelperDrawTextLine(wchar_t* str)
{
	if (!g_pRenderWindow || !g_pRenderWindow->m_pTextHelper)
		return;

	g_pRenderWindow->m_pTextHelper->DrawTextLine(str);
}

} // end namespace