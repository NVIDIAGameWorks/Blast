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
#include <DirectXTex.h>
#include "D3D12Util.h"

#include "D3D12Shaders.h"
#include "D3D12RenderShader.h"

#include "D3DX10tex.h"
#include "D3DX11tex.h"

#include "D3D12Wrapper.h"
#include "DXUT.h"
#include "DXUTgui.h"
#include "sdkmisc.h"
#include "D3D12RendererWindow.h"
#include "SimpleRenderable.h"
//#include "MeshShaderParam.h"
#include "D3D12RenderInterface.h"
#include "D3D12TextureResource.h"
#include "D3D12RenderContext.h"
namespace D3D12Util
{
	using namespace RenderInterface;

	// D3D hook to render window
	D3D12RenderWindow*		g_pRenderWindow = 0;

///////////////////////////////////////////////////////////////////////////////
GPUShaderResource*
	CreateTextureSRV(const char* texturename)
{
	D3D12RenderContext* pContext = D3D12RenderContext::Instance();
	if (!pContext)
		return 0;
	ID3D12Device* pDevice = pContext->GetDevice();
	if (!pDevice)
		return 0;
	ID3D12GraphicsCommandList* pCommandList = pContext->GetGraphicsCommandList();
	if (!pCommandList)
		return 0;

	unsigned char *pSTBIRes = 0;
	int width = 0;
	int height = 0;

	size_t nu = strlen(texturename);
	size_t n = (size_t)MultiByteToWideChar(CP_ACP, 0, (const char *)texturename, (int)nu, NULL, 0);
	wchar_t* pwstr = new wchar_t[n];
	MultiByteToWideChar(CP_ACP, 0, (const char *)texturename, (int)nu, pwstr, (int)n);
	pwstr[n] = 0;

	TexMetadata texMetadata;
	ScratchImage scratchImage;
	HRESULT loaded = LoadFromTGAFile(pwstr, &texMetadata, scratchImage);

	if (loaded != S_OK)
	{
		loaded = LoadFromWICFile(pwstr, TEX_FILTER_DEFAULT | WIC_FLAGS_ALL_FRAMES, &texMetadata, scratchImage);
	}

	if (loaded != S_OK)
	{
		loaded = LoadFromDDSFile(pwstr, DDS_FLAGS_NONE, &texMetadata, scratchImage);
	}

	if (loaded == S_OK)
	{
		pSTBIRes = scratchImage.GetPixels();
		width = texMetadata.width;
		height = texMetadata.height;
	}

	if (!pSTBIRes)
		return 0;

	int numMipMaps = 0;
	{
		int mipWidth = width;
		int mipHeight = height;
		while (mipWidth > 1 || mipHeight > 1)
		{
			numMipMaps++;
			mipWidth >>= 1;
			mipHeight >>= 1;

			if ((mipWidth * sizeof(uint32_t)) < D3D12_TEXTURE_DATA_PITCH_ALIGNMENT)
				break;
		}
	}

	std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(numMipMaps);
	std::vector<uint64_t> row_sizes_in_bytes(numMipMaps);
	std::vector<uint32_t> num_rows(numMipMaps);

	auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, width, height, 1, (UINT16)numMipMaps, 1, 0, D3D12_RESOURCE_FLAG_NONE,
		D3D12_TEXTURE_LAYOUT_UNKNOWN, 0);

	uint64_t required_size = 0;
	pDevice->GetCopyableFootprints(&resourceDesc, 0, numMipMaps, 0, &layouts[0], &num_rows[0], &row_sizes_in_bytes[0], &required_size);

	HRESULT hr;
	ID3D12Resource* mTextureUpload;
	ID3D12Resource* mTexture;

	hr = pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&mTexture));
	mTexture->SetName(L"Texture");
	hr = pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(required_size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mTextureUpload));
	mTextureUpload->SetName(L"TextureUpload");

	const int requestedMipLevels = numMipMaps;
	D3D12_SUBRESOURCE_DATA* initData = new D3D12_SUBRESOURCE_DATA[requestedMipLevels];
	ZeroMemory(initData, sizeof(D3D12_SUBRESOURCE_DATA)*requestedMipLevels);

	struct Pixel
	{
		unsigned char rgba[4];
	};

	// prepare target buffer just large enough to include all the mip levels
	Pixel* targets = new Pixel[width*height * 2];

	// copy the first mip level
	memcpy(targets, pSTBIRes, width*height * 4);

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
		initData[idx].pData = source;
		initData[idx].RowPitch = mipWidth * 4;
		mipLevels++;

		// skip generating mip for 1x1
		if ((mipWidth == 1) && (mipHeight == 1))
			break;

		// skip generating mip for the last level
		if (idx == (requestedMipLevels - 1))
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
				const int px = x * 2; // x in previous mip
				const int py = y * 2; // y in previous mip

				samples[0] = source[py*prevWidth + px]; // left top
				samples[1] = source[py*prevWidth + px + 1]; // right top
				samples[2] = source[(py + 1)*prevWidth + px]; // left bottom
				samples[3] = source[(py + 1)*prevWidth + px + 1]; // right bottom

																	// for each component
				for (int comp = 0; comp < 4; ++comp)
				{
					// do the linear box filter for lower mip level
					target[y*mipWidth + x].rgba[comp] = (samples[0].rgba[comp] + samples[1].rgba[comp] + samples[2].rgba[comp] + samples[3].rgba[comp]) / 4;
				}
			}
		}

		// update source
		source = target;
	}

	uint8_t* p;
	mTextureUpload->Map(0, nullptr, reinterpret_cast<void**>(&p));
	for (uint32_t i = 0; i < numMipMaps; ++i)
	{
		memcpy(p + layouts[i].Offset, initData[i].pData, layouts[i].Footprint.RowPitch * num_rows[i]);
	}
	mTextureUpload->Unmap(0, nullptr);

	for (uint32_t i = 0; i < numMipMaps; ++i)
	{
		D3D12_TEXTURE_COPY_LOCATION src;
		src.pResource = mTextureUpload;
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.PlacedFootprint = layouts[i];

		D3D12_TEXTURE_COPY_LOCATION dst;
		dst.pResource = mTexture;
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst.SubresourceIndex = i;
		pCommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
	}
	pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));

	delete initData;
	delete targets;

	int nIndexInHeap = -1;
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle = pContext->NVHairINT_CreateD3D12Texture(mTexture, nIndexInHeap);
	return D3D12TextureResource::Create(mTexture, handle, nIndexInHeap);
}


///////////////////////////////////////////////////////////////////////////////
bool GetDeviceInfoString(wchar_t *str)
{
	ID3D11Device* pDevice = nullptr;// RenderInterfaceD3D12::GetDevice();
	if (!pDevice)
		return false;

	{
		IDXGIDevice1 *pDXGIDevice1 = NULL;
		pDevice->QueryInterface(__uuidof(IDXGIDevice1), reinterpret_cast<void **>(&pDXGIDevice1));

		IDXGIAdapter1 *pDXGIAdapter1 = NULL;
		pDXGIDevice1->GetParent(__uuidof(IDXGIAdapter1), reinterpret_cast<void **>(&pDXGIAdapter1));

		if (pDXGIAdapter1)
		{
			auto adapterDescription = DXGI_ADAPTER_DESC1();
			pDXGIAdapter1->GetDesc1(&adapterDescription);

			WCHAR* pDescStr = adapterDescription.Description;
			float memInGB = float(adapterDescription.DedicatedVideoMemory) / 1e9f;
			swprintf_s(str, 1000, L"%s(%.1fGb)\n", pDescStr, memInGB);
		}
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Render window interafce
/////////////////////////////////////////////////////////////////////////////////////////
bool CreateRenderWindow(HWND hWnd, int nSamples)
{
	SAFE_DELETE(g_pRenderWindow);

	g_pRenderWindow = new D3D12RenderWindow;
	return g_pRenderWindow->Create(hWnd, nSamples);
}

D3DHandles& GetDeviceHandles(D3DHandles& deviceHandles)
{
	/*
	deviceHandles.pAdapter = RenderInterfaceD3D11::GetAdapter();
	deviceHandles.pFactory = RenderInterfaceD3D11::GetDXGIFactory();
	deviceHandles.pDevice = RenderInterfaceD3D11::GetDevice();
	deviceHandles.pDeviceContext = RenderInterfaceD3D11::GetDeviceContext();

	deviceHandles.pDXGISwapChain = g_pRenderWindow->m_pDXGISwapChain;
	deviceHandles.pD3D11BackBuffer = g_pRenderWindow->m_pD3D11BackBuffer;
	deviceHandles.pD3D11RenderTargetView = g_pRenderWindow->m_pD3D11RenderTargetView;
	deviceHandles.pD3D11DepthBuffer = g_pRenderWindow->m_pD3D11DepthBuffer;
	deviceHandles.pD3D11DepthStencilView = g_pRenderWindow->m_pD3D11DepthStencilView;
	*/
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