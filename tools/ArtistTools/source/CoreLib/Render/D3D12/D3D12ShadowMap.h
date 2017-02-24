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

#include "ShadowMap.h"

#include <d3d11.h>
#include <d3d12.h>
#include "D3D12TextureResource.h"

#include "DXUT.h" // DXUT header
#include "d3dx12.h"

#include "D3D12RenderTarget.h"

using namespace Microsoft::WRL;

class GPUShaderResource;
class D3D12TextureResource;

struct D3D12ShadowMap : public ShadowMap
{
	D3D12TextureResource m_ShadowResource;

	D3D12RenderTarget* m_pRenderTarget;
	
public:
	D3D12ShadowMap(int resolution );
	~D3D12ShadowMap();

	void Release();
	void BeginRendering(float clearDepth);
	void EndRendering();

	GPUShaderResource* GetShadowSRV();

protected:

	bool isValid();
	int m_nIndex;
};

