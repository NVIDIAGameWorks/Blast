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

#include "D3D11RenderShader.h"

#include "D3D11RenderInterface.h"
#include "D3D11Wrapper.h"
#include "Nv.h"

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) { if (x) x->Release(); }
#endif

///////////////////////////////////////////////////////////////////////////////
D3D11RenderShader::D3D11RenderShader() :
	m_pVertexShader(0),
	m_pPixelShader(0),
	m_pInputLayout(0)
{
	for (int i = 0; i < D3D11_COMMONSHADER_CONSTANT_BUFFER_HW_SLOT_COUNT; i++)
	{
		m_pParamBuffers[i] = nullptr;
	}
}

///////////////////////////////////////////////////////////////////////////////
D3D11RenderShader::~D3D11RenderShader()
{
	SAFE_RELEASE(m_pVertexShader);
	SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pInputLayout);

	for (int i = 0; i < D3D11_COMMONSHADER_CONSTANT_BUFFER_HW_SLOT_COUNT; i++)
		SAFE_RELEASE(m_pParamBuffers[i]);
}

///////////////////////////////////////////////////////////////////////////////
D3D11RenderShader* D3D11RenderShader::Create(
	const char *name, 
	void* pVSBlob, size_t vsBlobSize, 
	void* pPSBlob, size_t psBlobSize, 
	UINT  cbufferSize0,	UINT  cbufferSize1,
	D3D11_INPUT_ELEMENT_DESC* pElemDesc, UINT numElements)
{
	D3D11RenderShader* pShader = new D3D11RenderShader;

	pShader->CreateVSFromBlob(pVSBlob, vsBlobSize, pElemDesc, numElements);
	pShader->CreatePSFromBlob(pPSBlob, psBlobSize);

	if (cbufferSize0 > 0)
	{
		pShader->CreateParamBuffer(cbufferSize0, 0);
		SET_D3D_DEBUG_NAME(pShader->getParamBuffer(0), name);
	}

	if (cbufferSize1 > 0)
	{
		pShader->CreateParamBuffer(cbufferSize1, 1);
		SET_D3D_DEBUG_NAME(pShader->getParamBuffer(1), name);
	}

	if (pShader->getVertexShader())
		SET_D3D_DEBUG_NAME(pShader->getVertexShader(), name);

	if (pShader->getPixelShader())
		SET_D3D_DEBUG_NAME(pShader->getPixelShader(), name);

	if (pShader->getInputLayout())
		SET_D3D_DEBUG_NAME(pShader->getInputLayout(), name);

	return pShader;
}

///////////////////////////////////////////////////////////////////////////////
bool D3D11RenderShader::CreateVSFromBlob(
	void* pBlob, size_t blobSize, 
	D3D11_INPUT_ELEMENT_DESC *desc, int elemCount)
{
	ID3D11Device *pDevice = RenderInterfaceD3D11::GetDevice();
	if (!pDevice)
		return false;

	if (!pBlob || (blobSize == 0))
		return false;

	SAFE_RELEASE(m_pVertexShader);
	SAFE_RELEASE(m_pInputLayout);

	HRESULT res = pDevice->CreateVertexShader(pBlob, blobSize, NV_NULL, &m_pVertexShader);

	if(FAILED(res)) return false;

	if (desc)
	{
		res = pDevice->CreateInputLayout(desc, elemCount, 
			pBlob, blobSize, &m_pInputLayout);

		if(FAILED(res)) return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
bool D3D11RenderShader::CreatePSFromBlob(void* pBlob, size_t blobSize)
{
	ID3D11Device* pDevice = RenderInterfaceD3D11::GetDevice();
	if (!pDevice)
		return false;

	if (!pBlob || (blobSize == 0))
		return false;

	SAFE_RELEASE(m_pPixelShader);

	HRESULT res = pDevice->CreatePixelShader(pBlob, blobSize, NV_NULL, &m_pPixelShader);

	if (FAILED(res)) 
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
void D3D11RenderShader::SetConstantBuffer()
{
	ID3D11DeviceContext* pContext = RenderInterfaceD3D11::GetDeviceContext();

	//for (int i = 0; i < D3D11_COMMONSHADER_CONSTANT_BUFFER_HW_SLOT_COUNT; i++)
	for (int i = 0; i < 2; i++)
	{
		pContext->PSSetConstantBuffers(i, 1, &m_pParamBuffers[i]);
	}
}

///////////////////////////////////////////////////////////////////////////////
void D3D11RenderShader::MakeCurrent()
{
	ID3D11DeviceContext* pContext = RenderInterfaceD3D11::GetDeviceContext();

	if (m_pInputLayout)
		pContext->IASetInputLayout(m_pInputLayout);
	else
		pContext->IASetInputLayout(NV_NULL);


	if	(m_pVertexShader)
	{
		pContext->VSSetShader( m_pVertexShader, NV_NULL, 0);
		if (m_pParamBuffers[0])
			pContext->VSSetConstantBuffers(0, 1, &m_pParamBuffers[0]);
	}

	if (m_pPixelShader)
	{
		pContext->PSSetShader( m_pPixelShader, NV_NULL, 0);
		if (m_pParamBuffers[0])
			pContext->PSSetConstantBuffers(0, 1, &m_pParamBuffers[0]);
		if (m_pParamBuffers[1])
			pContext->PSSetConstantBuffers(1, 1, &m_pParamBuffers[1]);
	}

	pContext->GSSetShader( 0, 0, 0);
	pContext->DSSetShader( 0, 0, 0);
	pContext->HSSetShader( 0, 0, 0);
}

///////////////////////////////////////////////////////////////////////////////
void D3D11RenderShader::Disable()
{
	ID3D11DeviceContext* pContext = RenderInterfaceD3D11::GetDeviceContext();

	pContext->IASetInputLayout(NV_NULL);

	pContext->VSSetShader( 0, NV_NULL, 0);
	pContext->PSSetShader( 0, NV_NULL, 0);
	pContext->GSSetShader( 0, 0, 0);
	pContext->DSSetShader( 0, 0, 0);
	pContext->HSSetShader( 0, 0, 0);
}

///////////////////////////////////////////////////////////////////////////////
bool D3D11RenderShader::CreateParamBuffer( UINT sizeBuffer, UINT slot )
{
	ID3D11Device* pDevice = RenderInterfaceD3D11::GetDevice();
	if (!pDevice)
		return false;

	SAFE_RELEASE(m_pParamBuffers[slot]);

	D3D11_BUFFER_DESC Desc;
	{
		Desc.Usage = D3D11_USAGE_DYNAMIC;
		Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		Desc.MiscFlags = 0;
		Desc.ByteWidth = sizeBuffer;
	}

	HRESULT hr = pDevice->CreateBuffer( &Desc, NV_NULL, &m_pParamBuffers[slot] );
	if( FAILED(hr) )
		false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
void* D3D11RenderShader::MapParam(UINT slot)
{
	ID3D11DeviceContext* pContext = RenderInterfaceD3D11::GetDeviceContext();

	if (!m_pParamBuffers[slot] || !pContext)
		return 0;

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT hr = pContext->Map(m_pParamBuffers[slot], 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	
	if ( FAILED(hr))
		return 0;

	return mappedResource.pData;
}

///////////////////////////////////////////////////////////////////////////////
void D3D11RenderShader::UnmapParam( UINT slot )
{
	ID3D11DeviceContext* pContext = RenderInterfaceD3D11::GetDeviceContext();

	if (pContext && m_pParamBuffers[slot])
		pContext->Unmap( m_pParamBuffers[slot], 0);
}