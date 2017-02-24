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
#include "D3D12GPUProfiler.h"

#include "D3D12RenderInterface.h"
#include "D3D12RenderContext.h"
///////////////////////////////////////////////////////////////
// factory function to create D3D12 GPU profiler
/*
GPUProfiler* GPUProfiler::CreateD3D12()
{
	GPUProfiler* pProfiler = new D3D12GPUProfiler;
	pProfiler->Initialize();
	return pProfiler;
}
*/

///////////////////////////////////////////////////////////////
D3D12GPUProfiler::~D3D12GPUProfiler()
{
	Release();
}

///////////////////////////////////////////////////////////////
void D3D12GPUProfiler::Initialize()
{
	D3D12RenderContext* pContext = D3D12RenderContext::Instance();

	/*
	ID3D12Device* pDevice12 = pContext->GetDevice();

	D3D12_QUERY_HEAP_DESC queryHeapDesc = {};
	queryHeapDesc.Count = MAX_QUERY_COUNT * 2;
	queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
	ThrowIfFailed(pDevice12->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&query_heap)));

	D3D12_HEAP_PROPERTIES heap_prop;
	heap_prop.Type = D3D12_HEAP_TYPE_DEFAULT;
	heap_prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heap_prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heap_prop.CreationNodeMask = 0;
	heap_prop.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC res_desc;
	res_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	res_desc.Alignment = 0;
	res_desc.Width = sizeof(UINT64) * 2;
	res_desc.Height = 1;
	res_desc.DepthOrArraySize = 1;
	res_desc.MipLevels = 1;
	res_desc.Format = DXGI_FORMAT_UNKNOWN;
	res_desc.SampleDesc.Count = 1;
	res_desc.SampleDesc.Quality = 0;
	res_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	res_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ThrowIfFailed(pDevice12->CreateCommittedResource(&heap_prop, D3D12_HEAP_FLAG_NONE,
		&res_desc, D3D12_RESOURCE_STATE_COMMON, nullptr,
		IID_ID3D12Resource, reinterpret_cast<void**>(&query_result)));

	heap_prop.Type = D3D12_HEAP_TYPE_READBACK;
	ThrowIfFailed(pDevice12->CreateCommittedResource(&heap_prop, D3D12_HEAP_FLAG_NONE,
		&res_desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
		IID_ID3D12Resource, reinterpret_cast<void**>(&query_result_readback)));
	*/

	m_pContext = pContext->GetDeviceContext();

	D3D11_QUERY_DESC desc;
	memset(&desc, 0, sizeof(D3D11_QUERY_DESC)); 
	desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
	desc.MiscFlags = 0;
	
	ID3D11Device* pDevice = pContext->GetDevice11();
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
void D3D12GPUProfiler::Release()
{
	for (int i = 0; i < MAX_QUERY_COUNT; i++)
	{
		SAFE_RELEASE(m_pQueryStart[i]);
		SAFE_RELEASE(m_pQueryEnd[i]);
	}

	SAFE_RELEASE(m_pQueryDisjoint);
}

///////////////////////////////////////////////////////////////
void D3D12GPUProfiler::StartProfile(int id)
{
	if (!m_enable) return;

	/*
	D3D12RenderContext* pContext = D3D12RenderContext::Instance();
	ID3D12GraphicsCommandList* pCommandList = pContext->GetGraphicsCommandList();
	pCommandList->EndQuery(query_heap, D3D12_QUERY_TYPE_TIMESTAMP, id * 2);
	*/

	ID3D11Query* pQuery = m_pQueryStart[id];	
	m_pContext->End(pQuery);
}

///////////////////////////////////////////////////////////////
void D3D12GPUProfiler::EndProfile(int id)
{
	if (!m_enable) return;

	/*
	D3D12RenderContext* pContext = D3D12RenderContext::Instance();
	ID3D12GraphicsCommandList* pCommandList = pContext->GetGraphicsCommandList();
	pCommandList->EndQuery(query_heap, D3D12_QUERY_TYPE_TIMESTAMP, id * 2 + 1);
	*/

	ID3D11Query* pQuery = m_pQueryEnd[id];	
	m_pContext->End(pQuery);
}

///////////////////////////////////////////////////////////////
void D3D12GPUProfiler::StartFrame()
{
	if (!m_enable) return;

	m_pContext->Begin(m_pQueryDisjoint);
}

///////////////////////////////////////////////////////////////
void D3D12GPUProfiler::EndFrame()
{
	if (!m_enable) return;

	m_pContext->End(m_pQueryDisjoint);
		
	while(m_pContext->GetData(m_pQueryDisjoint, &m_disjointData, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), 0) != S_OK);
}

///////////////////////////////////////////////////////////////
float D3D12GPUProfiler::GetProfileData(int id)
{
	if (!m_enable) 
		return 0.0f;

	/*
	D3D12RenderContext* pContext = D3D12RenderContext::Instance();
	ID3D12GraphicsCommandList* pCommandList = pContext->GetGraphicsCommandList();

	UINT64 times[2];
	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = query_result;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.Subresource = 0;
	pCommandList->ResourceBarrier(1, &barrier);

	pCommandList->ResolveQueryData(query_heap, D3D12_QUERY_TYPE_TIMESTAMP, id * 2, 2, query_result, 0);

	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
	pCommandList->ResourceBarrier(1, &barrier);

	pCommandList->CopyResource(query_result_readback, query_result);

	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
	pCommandList->ResourceBarrier(1, &barrier);

	void* pData;
	ThrowIfFailed(query_result_readback->Map(0, 0, &pData));
	memcpy(times, pData, sizeof(UINT64) * 2);
	query_result_readback->Unmap(0, 0);
	*/

	UINT64 startTime = 0;
	while(m_pContext->GetData(m_pQueryStart[id], &startTime, sizeof(UINT64), 0) != S_OK);

	UINT64 endTime = 0;
	while(m_pContext->GetData(m_pQueryEnd[id], &endTime, sizeof(UINT64), 0) != S_OK);
	
	float frequency = static_cast<float>(m_disjointData.Frequency);
	return (endTime - startTime) / frequency * 1000.0f;
}
