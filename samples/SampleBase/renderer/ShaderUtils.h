/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef SHADER_UTILS_H
#define SHADER_UTILS_H

#include "Utils.h"
#include <d3dcompiler.h>


static HRESULT CompileShaderFromFile(const char* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel,
                                     ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;
	ID3DBlob* pErrorBlob = NULL;

	WCHAR wFileName[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, szFileName, -1, wFileName, MAX_PATH);
	wFileName[MAX_PATH - 1] = 0;
	hr = D3DCompileFromFile(wFileName, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, szEntryPoint, szShaderModel, D3D10_SHADER_ENABLE_STRICTNESS, 0,
	                        ppBlobOut, &pErrorBlob);
	if(FAILED(hr))
	{
		OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		SAFE_RELEASE(pErrorBlob);
		return hr;
	}
	SAFE_RELEASE(pErrorBlob);

	return S_OK;
}

static HRESULT createShader(ID3D11Device* pDev, const void* pData, size_t len, ID3D11VertexShader** ppShd, bool)
{
	return pDev->CreateVertexShader(pData, len, nullptr, ppShd);
}

static HRESULT createShader(ID3D11Device* pDev, const void* pData, size_t len, ID3D11GeometryShader** ppShd,
                            bool forceFast)
{
	PX_UNUSED(forceFast);
	return pDev->CreateGeometryShader(pData, len, nullptr, ppShd);
}

static HRESULT createShader(ID3D11Device* pDev, const void* pData, size_t len, ID3D11PixelShader** ppShd, bool)
{
	return pDev->CreatePixelShader(pData, len, nullptr, ppShd);
}

static const char* shaderModel(ID3D11VertexShader**)
{
	return "vs_5_0";
}

static const char* shaderModel(ID3D11GeometryShader**)
{
	return "gs_5_0";
}

static const char* shaderModel(ID3D11PixelShader**)
{
	return "ps_5_0";
}

// Give back the shader buffer blob for use in CreateVertexLayout.  Caller must release the blob.
template <class S>
static HRESULT createShaderFromFile(ID3D11Device* pDev, const char* szFileName, LPCSTR szEntryPoint, S** ppShd,
                                    ID3DBlob*& pShaderBuffer, bool forceFast = false)
{
	HRESULT hr = CompileShaderFromFile(szFileName, szEntryPoint, shaderModel(ppShd), &pShaderBuffer);
	if(SUCCEEDED(hr) && pShaderBuffer)
	{
		const void* shaderBufferData = pShaderBuffer->GetBufferPointer();
		const UINT shaderBufferSize = pShaderBuffer->GetBufferSize();
		createShader(pDev, shaderBufferData, shaderBufferSize, ppShd, forceFast);
	}
	return hr;
}

// Overloaded, same as above but don't give back the shader buffer blob.
template <class S>
static HRESULT createShaderFromFile(ID3D11Device* pDev, const char* szFileName, LPCSTR szEntryPoint, S** ppShd,
                                    bool forceFast = false)
{
	ID3DBlob* pShaderBuffer = NULL;
	HRESULT hr = createShaderFromFile(pDev, szFileName, szEntryPoint, ppShd, pShaderBuffer, forceFast);
	SAFE_RELEASE(pShaderBuffer);
	return hr;
}


#endif //SHADER_UTILS_H