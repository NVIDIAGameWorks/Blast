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

#include "windows.h"

#include "RenderResources.h"

// API agnostic interface for rendering
namespace RenderInterface
{
	///////////////////////////////////////////////////////////////////
	enum RASTERIZER_STATE
	{
		RASTERIZER_STATE_FILL_CULL_NONE,
		RASTERIZER_STATE_FILL_CULL_FRONT,
		RASTERIZER_STATE_FILL_CULL_BACK,
		RASTERIZER_STATE_WIRE,
		RASTERIZER_STATE_END
	};

	///////////////////////////////////////////////////////////////////
	enum DEPTH_STENCIL_STATE
	{
		DEPTH_STENCIL_DEPTH_TEST,
		DEPTH_STENCIL_DEPTH_NONE,
		DEPTH_STENCIL_STATE_END
	};

	///////////////////////////////////////////////////////////////////
	enum SAMPLER_TYPE
	{
		SAMPLER_TYPE_LINEAR,
		SAMPLER_TYPE_POINTCLAMP,
		SAMPLER_TYPE_END,
	};

	///////////////////////////////////////////////////////////////////
	enum BLEND_STATE
	{
		BLEND_STATE_ALPHA,
		BLEND_STATE_NONE,
		BLEND_STATE_END
	};

	///////////////////////////////////////////////////////////////////
	enum SHADER_TYPE
	{
		SHADER_TYPE_MESH_RENDERING,
		SHADER_TYPE_MESH_SHADOW,
		SHADER_TYPE_SCREEN_QUAD,
		SHADER_TYPE_SCREEN_QUAD_COLOR,
		SHADER_TYPE_VISUALIZE_SHADOW,
		SHADER_TYPE_SIMPLE_COLOR,
#ifndef NV_ARTISTTOOLS
		SHADER_TYPE_HAIR_SHADER_DEFAULT,
		SHADER_TYPE_HAIR_SHADER_SHADOW,
#endif // NV_ARTISTTOOLS
	};

	///////////////////////////////////////////////////////////////////
	// global acess for render context and device to minimize D3D entry points
	///////////////////////////////////////////////////////////////////
	CORELIB_EXPORT bool InitDevice(int deviceID);
	CORELIB_EXPORT bool Initialize();
	CORELIB_EXPORT void Shutdown();

	CORELIB_EXPORT void SubmitGpuWork();
	CORELIB_EXPORT void WaitForGpu();

	///////////////////////////////////////////////////////////////////
	// render window management
	///////////////////////////////////////////////////////////////////
	CORELIB_EXPORT bool CreateRenderWindow(HWND hWnd, int nSamples);
	CORELIB_EXPORT bool ResizeRenderWindow(int w, int h);
	CORELIB_EXPORT void PresentRenderWindow();
	CORELIB_EXPORT void ClearRenderWindow(float r, float g, float b);

	///////////////////////////////////////////////////////////////////
	// shader magement
	///////////////////////////////////////////////////////////////////
	CORELIB_EXPORT void ApplyShader(SHADER_TYPE st);
	CORELIB_EXPORT void CopyShaderParam(SHADER_TYPE st, void* pSysMem, unsigned int bytes, unsigned int slot = 0);

	///////////////////////////////////////////////////////////////////
	// viewport management
	///////////////////////////////////////////////////////////////////
	struct Viewport
	{
		float			TopLeftX;
		float			TopLeftY;
		float			Width;
		float			Height;
		float			MinDepth;
		float			MaxDepth;
	};

	CORELIB_EXPORT void GetViewport(Viewport& vp);
	CORELIB_EXPORT void SetViewport(const Viewport& vp);

	///////////////////////////////////////////////////////////////////
	// gpu buffer management
	///////////////////////////////////////////////////////////////////
	CORELIB_EXPORT GPUBufferResource* CreateVertexBuffer( 	unsigned int ByteWidth, void* pSysMem = 0);
	CORELIB_EXPORT void CopyToDevice(GPUBufferResource *pDevicePtr, void* pSysMem, unsigned int	ByteWidth);

	///////////////////////////////////////////////////////////////////
	// texture resource mangement
	///////////////////////////////////////////////////////////////////
	CORELIB_EXPORT GPUShaderResource* CreateTextureResource(const char* filePath);
	CORELIB_EXPORT void RenderShadowMap(GPUShaderResource* pShadowSRV, float znear, float zfar);

	///////////////////////////////////////////////////////////////////
	// render state management
	///////////////////////////////////////////////////////////////////
	CORELIB_EXPORT void ApplyDepthStencilState(DEPTH_STENCIL_STATE state);
	CORELIB_EXPORT void ApplyRasterizerState(RASTERIZER_STATE state);
	CORELIB_EXPORT void ApplySampler(int slot, SAMPLER_TYPE st);
	CORELIB_EXPORT void ApplyBlendState(BLEND_STATE st);

	CORELIB_EXPORT void ApplyForShadow(int ForShadow);
	CORELIB_EXPORT void SwitchToDX11();
	CORELIB_EXPORT void FlushDX11();
	CORELIB_EXPORT void FlushDX12();
	CORELIB_EXPORT void ApplyPrimitiveTopologyLine();
	CORELIB_EXPORT void ApplyPrimitiveTopologyTriangle();
	///////////////////////////////////////////////////////////////////
	// draw calls
	///////////////////////////////////////////////////////////////////
	void DrawLineList(GPUBufferResource* pVertexBuffer, unsigned int nVerts, unsigned int bytesize);

	///////////////////////////////////////////////////////////////////
	// background textures
	///////////////////////////////////////////////////////////////////
	bool LoadBackgroundTexture(const char* filePath);
	void RenderBackgroundTexture();
	void ClearBackgroundTexture();

	///////////////////////////////////////////////////////////////////
	CORELIB_EXPORT bool GetDeviceInfoString(wchar_t *str);

	///////////////////////////////////////////////////////////////////
	// text helpers
	CORELIB_EXPORT void TxtHelperBegin();
	CORELIB_EXPORT void TxtHelperEnd();
	CORELIB_EXPORT void TxtHelperSetInsertionPos(int x, int y);
	CORELIB_EXPORT void TxtHelperSetForegroundColor(float r, float g, float b, float a = 1.0f);
	CORELIB_EXPORT void TxtHelperDrawTextLine(wchar_t* str);
}
