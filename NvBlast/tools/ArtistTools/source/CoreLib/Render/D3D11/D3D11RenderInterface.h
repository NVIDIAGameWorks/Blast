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

#pragma once

#include "d3d11.h"

#include "RenderInterface.h"

#include "RenderPlugin.h"

#ifdef NV_ARTISTTOOLS
enum HAIR_SHADER_TYPE
{
	SHADER_TYPE_HAIR_SHADER_DEFAULT = RenderInterface::SHADER_TYPE_SIMPLE_COLOR + 1,
	SHADER_TYPE_HAIR_SHADER_SHADOW,
};
#endif

// abstract interface to D3D calls
namespace RenderInterfaceD3D11
{
	CORERENDER_EXPORT bool InitDevice(int deviceID);
	CORERENDER_EXPORT bool Initialize();
	CORERENDER_EXPORT void Shutdown();

	CORERENDER_EXPORT ID3D11DeviceContext* GetDeviceContext();
	CORERENDER_EXPORT ID3D11Device* GetDevice();
	IDXGIFactory1* GetDXGIFactory();
	IDXGIAdapter* GetAdapter();

	CORERENDER_EXPORT void InitializeRenderStates();
	CORERENDER_EXPORT void ClearRenderStates();

	CORERENDER_EXPORT void ApplySampler(int slot, RenderInterface::SAMPLER_TYPE st);
	CORERENDER_EXPORT void ApplyDepthStencilState(RenderInterface::DEPTH_STENCIL_STATE st);
	CORERENDER_EXPORT void ApplyRasterizerState(RenderInterface::RASTERIZER_STATE st);
	CORERENDER_EXPORT void ApplyBlendState(RenderInterface::BLEND_STATE st);

	CORERENDER_EXPORT void GetViewport(RenderInterface::Viewport& vp);
	CORERENDER_EXPORT void SetViewport(const RenderInterface::Viewport& vp);

	CORERENDER_EXPORT void BindVertexShaderResources( int startSlot, int numSRVs, GPUShaderResource** ppSRVs);
	CORERENDER_EXPORT void BindPixelShaderResources( int startSlot, int numSRVs, GPUShaderResource** ppSRVs);

	CORERENDER_EXPORT void ClearVertexShaderResources( int startSlot, int numSRVs);
	CORERENDER_EXPORT void ClearPixelShaderResources( int startSlot, int numSRVs);

	CORERENDER_EXPORT GPUBufferResource* CreateVertexBuffer( unsigned int ByteWidth, void* pSysMem);
	CORERENDER_EXPORT GPUShaderResource* CreateShaderResource( unsigned int stride, unsigned int numElements, void* pSysMem );

	CORERENDER_EXPORT void CopyToDevice(GPUBufferResource *pDevicePtr, void* pSysMem, unsigned int ByteWidth);

	CORERENDER_EXPORT void ClearInputLayout();

	CORERENDER_EXPORT void SetVertexBuffer(GPUBufferResource* pBuffer, UINT stride, UINT offset = 0);

	CORERENDER_EXPORT void SetPrimitiveTopologyTriangleStrip();
	CORERENDER_EXPORT void SetPrimitiveTopologyTriangleList();
	CORERENDER_EXPORT void SetPrimitiveTopologyLineList();

	
	CORERENDER_EXPORT void Draw(unsigned int vertexCount, unsigned int startCount = 0);
}

