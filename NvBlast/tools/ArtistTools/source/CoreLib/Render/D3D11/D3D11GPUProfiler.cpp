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
#include "D3D11GPUProfiler.h"

#include "D3D11RenderInterface.h"

///////////////////////////////////////////////////////////////
// factory function to create D3D11 GPU profiler
/*
GPUProfiler* GPUProfiler::CreateD3D11()
{
	GPUProfiler* pProfiler = new D3D11GPUProfiler;
	pProfiler->Initialize();
	return pProfiler;
}
*/
///////////////////////////////////////////////////////////////
D3D11GPUProfiler::~D3D11GPUProfiler()
{
	Release();
}

///////////////////////////////////////////////////////////////
void D3D11GPUProfiler::Initialize()
{
	ID3D11Device *pDevice = RenderInterfaceD3D11::GetDevice();
	
	m_pContext = RenderInterfaceD3D11::GetDeviceContext();

	D3D11_QUERY_DESC desc;
	memset(&desc, 0, sizeof(D3D11_QUERY_DESC)); 
	desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
	desc.MiscFlags = 0;

	pDevice->CreateQuery(&desc, &m_pQueryDisjoint);

	desc.Query = D3D11_QUERY_TIMESTAMP;

	for (int i = 0; i < MAX_QUERY_COUNT; i++)
	{
		pDevice->CreateQuery(&desc, &m_pQueryStart[i]);
		pDevice->CreateQuery(&desc, &m_pQueryEnd[i]);
	}
	m_enable = true;
}

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) { if (x) x->Release(); x = 0; }
#endif

///////////////////////////////////////////////////////////////
void D3D11GPUProfiler::Release()
{
	for (int i = 0; i < MAX_QUERY_COUNT; i++)
	{
		SAFE_RELEASE(m_pQueryStart[i]);
		SAFE_RELEASE(m_pQueryEnd[i]);
	}

	SAFE_RELEASE(m_pQueryDisjoint);
}

///////////////////////////////////////////////////////////////
void D3D11GPUProfiler::StartProfile(int id)
{
	if (!m_enable) return;

	ID3D11Query* pQuery = m_pQueryStart[id];	
	m_pContext->End(pQuery);
}

///////////////////////////////////////////////////////////////
void D3D11GPUProfiler::EndProfile(int id)
{
	if (!m_enable) return;

	ID3D11Query* pQuery = m_pQueryEnd[id];	
	m_pContext->End(pQuery);
}

///////////////////////////////////////////////////////////////
void D3D11GPUProfiler::StartFrame()
{
	if (!m_enable) return;

	m_pContext->Begin(m_pQueryDisjoint);
}

///////////////////////////////////////////////////////////////
void D3D11GPUProfiler::EndFrame()
{
	if (!m_enable) return;

	m_pContext->End(m_pQueryDisjoint);
		
	while(m_pContext->GetData(m_pQueryDisjoint, &m_disjointData, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), 0) != S_OK);
}

///////////////////////////////////////////////////////////////
float D3D11GPUProfiler::GetProfileData(int id)
{
	if (!m_enable) 
		return 0.0f;

	UINT64 startTime = 0;
	while(m_pContext->GetData(m_pQueryStart[id], &startTime, sizeof(UINT64), 0) != S_OK);

	UINT64 endTime = 0;
	while(m_pContext->GetData(m_pQueryEnd[id], &endTime, sizeof(UINT64), 0) != S_OK);

	float frequency = static_cast<float>(m_disjointData.Frequency);
	return (endTime - startTime) / frequency * 1000.0f;
}
