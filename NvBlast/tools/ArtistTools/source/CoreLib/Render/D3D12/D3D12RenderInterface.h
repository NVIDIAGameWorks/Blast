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
#include <d3d11.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include "RenderInterface.h"
#include "RenderPlugin.h"
class RenderShaderState;
// abstract interface to D3D calls
namespace RenderInterfaceD3D12
{
	CORERENDER_EXPORT bool InitDevice(int deviceID);
	CORERENDER_EXPORT bool Initialize();
	CORERENDER_EXPORT void Shutdown();

	RenderShaderState* GetShaderState();
	void InitializeRenderStates();

	CORERENDER_EXPORT void ApplyDepthStencilState(RenderInterface::DEPTH_STENCIL_STATE st);
	CORERENDER_EXPORT void ApplyRasterizerState(RenderInterface::RASTERIZER_STATE st);
	CORERENDER_EXPORT void ApplyBlendState(RenderInterface::BLEND_STATE st);
	CORERENDER_EXPORT void ApplyPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType);
	CORERENDER_EXPORT void ApplyForShadow(int ForShadow);	
	CORERENDER_EXPORT void SwitchToDX11();
	CORERENDER_EXPORT void FlushDX11();
	CORERENDER_EXPORT void FlushDX12();
	CORERENDER_EXPORT void SubmitGpuWork();
	CORERENDER_EXPORT void WaitForGpu();

	CORERENDER_EXPORT void GetViewport(RenderInterface::Viewport& vp);
	CORERENDER_EXPORT void SetViewport(const RenderInterface::Viewport& vp);

	CORERENDER_EXPORT GPUBufferResource* CreateVertexBuffer( unsigned int ByteWidth, void* pSysMem);
	CORERENDER_EXPORT GPUShaderResource* CreateShaderResource( unsigned int stride, unsigned int numElements, void* pSysMem, NVHairReadOnlyBuffer* pReadOnlyBuffer);

	CORERENDER_EXPORT void CopyToDevice(GPUBufferResource *pDevicePtr, void* pSysMem, unsigned int ByteWidth);
	
	CORERENDER_EXPORT void SetVertexBuffer(GPUBufferResource* pBuffer, UINT stride, UINT offset = 0);

	CORERENDER_EXPORT void SetPrimitiveTopologyTriangleStrip();
	CORERENDER_EXPORT void SetPrimitiveTopologyTriangleList();
	CORERENDER_EXPORT void SetPrimitiveTopologyLineList();

	
	CORERENDER_EXPORT void Draw(unsigned int vertexCount, unsigned int startCount = 0);
}

