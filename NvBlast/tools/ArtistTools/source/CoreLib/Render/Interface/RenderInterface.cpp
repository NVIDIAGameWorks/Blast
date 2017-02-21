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
#include "RenderInterface.h"
#include "MeshShaderParam.h"
#include "RenderPlugin.h"

namespace RenderInterface
{

////////////////////////////////////////////////////////////////////////////////////////////
bool InitDevice(int deviceID)
{
	if (!RenderPlugin::Instance()->InitDevice(deviceID))
	{
		MessageBox( 0, L"Could not create device.", L"Error", MB_ICONEXCLAMATION );
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
bool GetDeviceInfoString(wchar_t *str)
{
	return RenderPlugin::Instance()->GetDeviceInfoString(str);
}

//////////////////////////////////////////////////////////////////////////
bool Initialize()
{
	return RenderPlugin::Instance()->Initialize();
}

///////////////////////////////////////////////////////////////////////////////
void Shutdown()
{
	RenderPlugin::Instance()->Shutdown();
}

///////////////////////////////////////////////////////////////////////////////
// gpu buffer management
///////////////////////////////////////////////////////////////////////////////
GPUBufferResource* CreateVertexBuffer( 
	unsigned int ByteWidth, void* pSysMem)
{
	return RenderPlugin::Instance()->CreateVertexBuffer(ByteWidth, pSysMem);
}

///////////////////////////////////////////////////////////////////////////////
void CopyToDevice(
	GPUBufferResource *pDevicePtr, 	void* pSysMem, 	unsigned int	ByteWidth)
{
	RenderPlugin::Instance()->CopyToDevice(pDevicePtr, pSysMem, ByteWidth);
}

///////////////////////////////////////////////////////////////////
// texture resource mangement
///////////////////////////////////////////////////////////////////
GPUShaderResource* CreateTextureResource(const char* filepath)
{
	return RenderPlugin::Instance()->CreateTextureSRV(filepath);
}

///////////////////////////////////////////////////////////////////////////////
void RenderShadowMap(GPUShaderResource* pShadowSRV, float znear, float zfar)
{
	// update constant buffer
	ShadowVizParam shaderParam;
	{
		shaderParam.m_zFar = zfar;
		shaderParam.m_zNear = znear;
	}
	CopyShaderParam(SHADER_TYPE_VISUALIZE_SHADOW, &shaderParam, sizeof(ShadowVizParam), 0);
	
	RenderPlugin::Instance()->BindPixelShaderResources(0, 1, &pShadowSRV);

	// render states
	RenderPlugin::Instance()->ApplySampler(0, SAMPLER_TYPE_POINTCLAMP);

	RenderPlugin::Instance()->BindShaderResources(SHADER_TYPE_VISUALIZE_SHADOW, 1, &pShadowSRV);


	RenderPlugin::Instance()->ApplyRasterizerState(RASTERIZER_STATE_FILL_CULL_NONE);
	RenderPlugin::Instance()->ApplyDepthStencilState(DEPTH_STENCIL_DEPTH_NONE);
	
	// set IA vars
	RenderPlugin::Instance()->ClearInputLayout();

	RenderPlugin::Instance()->SetPrimitiveTopologyTriangleStrip();

	// set shader and tex resource
	ApplyShader(SHADER_TYPE_VISUALIZE_SHADOW);

	// draw quad
	RenderPlugin::Instance()->Draw(3, 0);

	// cleanup shader and its resource
	RenderPlugin::Instance()->ClearPixelShaderResources(0, 1);
	RenderPlugin::Instance()->DisableShader(SHADER_TYPE_VISUALIZE_SHADOW);
}

///////////////////////////////////////////////////////////////////////////////
// render with full color shader
void RenderScreenQuad(
	GPUShaderResource*	pTextureSRV
	)
{
	RenderPlugin::Instance()->ApplyRasterizerState(RASTERIZER_STATE_FILL_CULL_NONE);
	RenderPlugin::Instance()->ApplyDepthStencilState(DEPTH_STENCIL_DEPTH_NONE);

	RenderPlugin::Instance()->ClearInputLayout();

	RenderPlugin::Instance()->SetPrimitiveTopologyTriangleStrip();

	if(pTextureSRV)
	{
		RenderPlugin::Instance()->BindShaderResources(SHADER_TYPE_SCREEN_QUAD, 1, &pTextureSRV);

		ApplyShader(SHADER_TYPE_SCREEN_QUAD);
		
		RenderPlugin::Instance()->BindPixelShaderResources(0, 1, &pTextureSRV);
		RenderPlugin::Instance()->ApplySampler(0, SAMPLER_TYPE_POINTCLAMP);

		RenderPlugin::Instance()->Draw(3,0);

		RenderPlugin::Instance()->ClearPixelShaderResources(0, 1);
		RenderPlugin::Instance()->DisableShader(SHADER_TYPE_SCREEN_QUAD);
	}
	else
	{
		ApplyShader(SHADER_TYPE_SCREEN_QUAD_COLOR);

		RenderPlugin::Instance()->Draw(3,0);

		RenderPlugin::Instance()->DisableShader(SHADER_TYPE_SCREEN_QUAD_COLOR);
	}
}


void SubmitGpuWork()
{
	RenderPlugin::Instance()->SubmitGpuWork();
}

void WaitForGpu()
{
	RenderPlugin::Instance()->WaitForGpu();
}


///////////////////////////////////////////////////////////////////////////////
// draw calls
///////////////////////////////////////////////////////////////////////////////
void DrawLineList(GPUBufferResource* pDevicePtr, unsigned int nVerts, unsigned int stride)
{
	RenderPlugin::Instance()->SetVertexBuffer(pDevicePtr, stride);
	RenderPlugin::Instance()->SetPrimitiveTopologyLineList();
	RenderPlugin::Instance()->Draw(nVerts, 0);
}

///////////////////////////////////////////////////////////////////////////////
// render states management
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
void ApplyDepthStencilState(DEPTH_STENCIL_STATE state)
{
	RenderPlugin::Instance()->ApplyDepthStencilState(state);
}

///////////////////////////////////////////////////////////////////////////////
void ApplyRasterizerState(RASTERIZER_STATE state)
{
	RenderPlugin::Instance()->ApplyRasterizerState(state);
}

///////////////////////////////////////////////////////////////////////////////
void ApplySampler(int slot, SAMPLER_TYPE st)
{
	RenderPlugin::Instance()->ApplySampler(slot, st);
}

///////////////////////////////////////////////////////////////////////////////
void ApplyBlendState(BLEND_STATE st)
{
	RenderPlugin::Instance()->ApplyBlendState(st);
}

void ApplyForShadow(int ForShadow)
{
	RenderPlugin::Instance()->ApplyForShadow(ForShadow);
}

void SwitchToDX11()
{
	RenderPlugin::Instance()->SwitchToDX11();
}

void FlushDX11()
{
	RenderPlugin::Instance()->FlushDX11();
}

void FlushDX12()
{
	RenderPlugin::Instance()->FlushDX12();
}

void ApplyPrimitiveTopologyLine()
{
	RenderPlugin::Instance()->ApplyPrimitiveTopologyLine();
}

void ApplyPrimitiveTopologyTriangle()
{
	RenderPlugin::Instance()->ApplyPrimitiveTopologyTriangle();
}

///////////////////////////////////////////////////////////////////
// shader magement
///////////////////////////////////////////////////////////////////
void ApplyShader(SHADER_TYPE st)
{
	RenderPlugin::Instance()->ApplyShader(st);
}

///////////////////////////////////////////////////////////////////////////////
void CopyShaderParam(SHADER_TYPE st, void* pSysMem, unsigned int bytes, unsigned int slot)
{
	RenderPlugin::Instance()->CopyShaderParam(st, pSysMem, bytes, slot);
}

/////////////////////////////////////////////////////////////////////////////////////////
// Viewport magement
/////////////////////////////////////////////////////////////////////////////////////////
void GetViewport(Viewport& vp)
{
	RenderPlugin::Instance()->GetViewport(vp);
}

///////////////////////////////////////////////////////////////////////////////
void SetViewport(const Viewport& vp)
{
	RenderPlugin::Instance()->SetViewport(vp);
}

/////////////////////////////////////////////////////////////////////////////////////////
// Render window interafce
/////////////////////////////////////////////////////////////////////////////////////////
bool CreateRenderWindow(HWND hWnd, int nSamples)
{
	return RenderPlugin::Instance()->CreateRenderWindow(hWnd, nSamples);
}

bool ResizeRenderWindow(int w, int h)
{
	return RenderPlugin::Instance()->ResizeRenderWindow(w,h);
}

void PresentRenderWindow()
{
	return RenderPlugin::Instance()->PresentRenderWindow();
}

void ClearRenderWindow(float r, float g, float b)
{
	return RenderPlugin::Instance()->ClearRenderWindow(r,g,b);
}

///////////////////////////////////////////////////////////////////////////////
// Texture resource management
///////////////////////////////////////////////////////////////////////////////
static GPUShaderResource* g_backgroundTextureSRV = 0;

bool LoadBackgroundTexture(const char* filePath)
{
	ClearBackgroundTexture();

	RenderPlugin::Instance()->PreRender();
	g_backgroundTextureSRV = RenderPlugin::Instance()->CreateTextureSRV(filePath);
	RenderPlugin::Instance()->PostRender();

	return (g_backgroundTextureSRV != 0);
}

///////////////////////////////////////////////////////////////////////////////
void RenderBackgroundTexture()
{
	RenderScreenQuad(g_backgroundTextureSRV);
}

///////////////////////////////////////////////////////////////////////////////
void ClearBackgroundTexture()
{
	SAFE_RELEASE(g_backgroundTextureSRV);
}

/////////////////////////////////////////////////////////////////////////////////////////
// Text draw helper functions (using DXUT)
/////////////////////////////////////////////////////////////////////////////////////////
void TxtHelperBegin()
{
	RenderPlugin::Instance()->TxtHelperBegin();
}

void TxtHelperEnd()
{
	RenderPlugin::Instance()->TxtHelperEnd();
}

void TxtHelperSetInsertionPos(int x, int y)
{
	RenderPlugin::Instance()->TxtHelperSetInsertionPos(x,y);
}

void TxtHelperSetForegroundColor(float r, float g, float b, float a)
{
	RenderPlugin::Instance()->TxtHelperSetForegroundColor(r,g,b,a);
}

void TxtHelperDrawTextLine(wchar_t* str)
{
	RenderPlugin::Instance()->TxtHelperDrawTextLine(str);
}

} // end namespace