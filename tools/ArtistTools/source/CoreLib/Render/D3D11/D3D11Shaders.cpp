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
#include "D3D11Shaders.h"

#include "MeshShaderParam.h"
#include "LightShaderParam.h"

//#include <Nv/Blast/NvHairSdk.h>
#include "D3D11RenderShader.h"

using namespace RenderInterface;

/////////////////////////////////////////////////////////////////////////////////////
// Common shader settings
//static D3D11RenderShader*	g_pShaders[SHADER_TYPE_END];
static std::map<int, D3D11RenderShader*> g_pShaders;
/*
namespace BodyShaderBlobs
{
	#include "Shaders/BodyShader_VS.h"
	#include "Shaders/BodyShader_PS.h"
}

namespace BodyShadowBlobs
{
	#include "Shaders/BodyShadow_VS.h"
	#include "Shaders/BodyShadow_PS.h"
}

namespace ScreenQuadBlobs
{
	#include "Shaders/ScreenQuad_VS.h"
	#include "Shaders/ScreenQuad_PS.h"
}

namespace ScreenQuadColorBlobs
{
	#include "Shaders/ScreenQuadColor_VS.h"
	#include "Shaders/ScreenQuadColor_PS.h"
}

namespace VisualizeShadowBlobs
{
	#include "Shaders/VisualizeShadow_VS.h"
	#include "Shaders/VisualizeShadow_PS.h"
}

namespace ColorBlobs
{
	#include "Shaders/Color_VS.h"
	#include "Shaders/Color_PS.h"
}

#ifndef NV_ARTISTTOOLS
namespace BlastShaderBlobs
{
#include "Shaders/BlastShader_PS.h"
}

namespace BlastShadowBlobs
{
#include "Shaders/BlastShadow_PS.h"
}
#endif // NV_ARTISTTOOLS
*/
//////////////////////////////////////////////////////////////////////////
bool InitializeShadersD3D11()
{
	/*
	D3D11_INPUT_ELEMENT_DESC layoutBodyRender[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	    { "VERTEX_NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "FACE_NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "VERTEX_ID", 0, DXGI_FORMAT_R32_FLOAT, 0, 56, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements = sizeof(layoutBodyRender)/sizeof(D3D11_INPUT_ELEMENT_DESC);

	g_pShaders[SHADER_TYPE_MESH_RENDERING] = D3D11RenderShader::Create( "MeshRenderShader",
		(void*)BodyShaderBlobs::g_vs_main, sizeof(BodyShaderBlobs::g_vs_main),
		(void*)BodyShaderBlobs::g_ps_main, sizeof(BodyShaderBlobs::g_ps_main),
		sizeof(MeshShaderParam), 0,
		&layoutBodyRender[0], numElements);

	g_pShaders[SHADER_TYPE_MESH_SHADOW] = D3D11RenderShader::Create( "MeshShadowShader",
		(void*)BodyShadowBlobs::g_vs_main, sizeof(BodyShadowBlobs::g_vs_main),
		(void*)BodyShadowBlobs::g_ps_main, sizeof(BodyShadowBlobs::g_ps_main),
		sizeof(MeshShadowShaderParam), 0,
		&layoutBodyRender[0], numElements);

	g_pShaders[SHADER_TYPE_SCREEN_QUAD] = D3D11RenderShader::Create( "ScreenQuadShader",
		(void*)ScreenQuadBlobs::g_vs_main, sizeof(ScreenQuadBlobs::g_vs_main),
		(void*)ScreenQuadBlobs::g_ps_main, sizeof(ScreenQuadBlobs::g_ps_main));

	g_pShaders[SHADER_TYPE_SCREEN_QUAD_COLOR] = D3D11RenderShader::Create( "ScreenQuadColorShader",
		(void*)ScreenQuadColorBlobs::g_vs_main, sizeof(ScreenQuadColorBlobs::g_vs_main),
		(void*)ScreenQuadColorBlobs::g_ps_main, sizeof(ScreenQuadColorBlobs::g_ps_main));

	g_pShaders[SHADER_TYPE_VISUALIZE_SHADOW] = D3D11RenderShader::Create( "VisualizeShadowShader",
		(void*)VisualizeShadowBlobs::g_vs_main, sizeof(VisualizeShadowBlobs::g_vs_main),
		(void*)VisualizeShadowBlobs::g_ps_main, sizeof(VisualizeShadowBlobs::g_ps_main),
		sizeof(ShadowVizParam));

	D3D11_INPUT_ELEMENT_DESC layout_Position_And_Color[] = 
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	    { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements2 = sizeof(layout_Position_And_Color)/sizeof(D3D11_INPUT_ELEMENT_DESC);

	g_pShaders[SHADER_TYPE_SIMPLE_COLOR] = D3D11RenderShader::Create( "Color",
		(void*)ColorBlobs::g_vs_main, sizeof(ColorBlobs::g_vs_main),
		(void*)ColorBlobs::g_ps_main, sizeof(ColorBlobs::g_ps_main),
		sizeof(SimpleShaderParam), 0,
		&layout_Position_And_Color[0], numElements2);

#ifndef NV_ARTISTTOOLS
	g_pShaders[SHADER_TYPE_HAIR_SHADER_DEFAULT] = D3D11RenderShader::Create(
		"hairShaderDefault",  0, 0,
		(void*)BlastShaderBlobs::g_ps_main, sizeof(BlastShaderBlobs::g_ps_main),
		sizeof(NvHair::ShaderConstantBuffer),
		sizeof(LightShaderParam)
		);

	g_pShaders[SHADER_TYPE_HAIR_SHADER_SHADOW] = D3D11RenderShader::Create(
		"hairShadow",  0, 0,
		(void*)BlastShadowBlobs::g_ps_main, sizeof(BlastShadowBlobs::g_ps_main),
		sizeof(NvHair::ShaderConstantBuffer),
		0);
#else
	CoreLib::Inst()->D3D11Shaders_InitializeShadersD3D11(g_pShaders);
#endif // NV_ARTISTTOOLS
	*/
	return true;
}

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) { if (x) x->Release(); x = 0; }
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(x) { if (x) delete x; x = 0; }
#endif

///////////////////////////////////////////////////////////////////////////////
void DestroyShadersD3D11()
{
	for (int i = 0; i < g_pShaders.size(); i++)
	{
		D3D11RenderShader*& pShader = g_pShaders[i];
		if (pShader)
		{
			delete pShader;
			pShader = 0;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
D3D11RenderShader* GetShaderD3D11(SHADER_TYPE st)
{
	return g_pShaders[st];
}

///////////////////////////////////////////////////////////////////////////////
void ApplyShaderD3D11(SHADER_TYPE st)
{
	D3D11RenderShader* pD3D11Shader = GetShaderD3D11(st);
	if (!pD3D11Shader)
		return;

	pD3D11Shader->MakeCurrent();
}

///////////////////////////////////////////////////////////////////////////////
void DisableShaderD3D11(RenderInterface::SHADER_TYPE st)
{
	D3D11RenderShader* pD3D11Shader = GetShaderD3D11(st);
	if (!pD3D11Shader)
		return;

	pD3D11Shader->Disable();
}

///////////////////////////////////////////////////////////////////////////////
void CopyShaderParamD3D11(SHADER_TYPE st, void* pSysMem, unsigned int bytes, unsigned int slot)
{
	D3D11RenderShader* pD3D11Shader = GetShaderD3D11(st);
	if (!pD3D11Shader)
		return;

	void* mappedParam = pD3D11Shader->MapParam(slot);

	memcpy(mappedParam, pSysMem, bytes);

	pD3D11Shader->UnmapParam(slot);
}