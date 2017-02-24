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

#pragma once

#include <d3d11.h>
/*
#include <d3d12.h>
*/
#include "GPUProfiler.h"

#define MAX_QUERY_COUNT 64
struct D3D12GPUProfiler : public GPUProfiler
{
public:
	~D3D12GPUProfiler();

	void Initialize();
	void Release();
	void StartProfile(int id);
	void EndProfile(int id);
	void StartFrame();
	void EndFrame();
	float GetProfileData(int id);

protected:
	ID3D11Query*	m_pQueryDisjoint;
	ID3D11Query*	m_pQueryStart[MAX_QUERY_COUNT];
	ID3D11Query*	m_pQueryEnd[MAX_QUERY_COUNT];
	D3D11_QUERY_DATA_TIMESTAMP_DISJOINT m_disjointData;
	ID3D11DeviceContext* m_pContext;
	/*
	ID3D12QueryHeap* query_heap;
	ID3D12Resource* query_result;
	ID3D12Resource* query_result_readback;
	*/
};

