// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.


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