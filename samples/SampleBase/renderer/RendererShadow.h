// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.


#ifndef RENDERER_SHADOW_H
#define RENDERER_SHADOW_H

#include <DirectXMath.h>
#include "Utils.h"
#include "gfsdk_shadowlib.h"

#include <string>


class CFirstPersonCamera;
class Renderer;

class RendererShadow
{
public:
	RendererShadow();
	~RendererShadow();

	void createResources(ID3D11Device *pd3dDevice, ID3D11DeviceContext* context, CFirstPersonCamera* camera);

	void setScreenResolution(float FovyRad, UINT Width, UINT Height, UINT uSampleCount, ID3D11DepthStencilView* pReadOnlyDSV);
	void changeShadowSettings(UINT Width, UINT Height, UINT uSampleCount, ID3D11DepthStencilView* pReadOnlyDSV);
	void renderShadowMaps(Renderer* renderer);
	void renderShadowBuffer(ID3D11ShaderResourceView* pDepthStencilSRV, ID3D11ShaderResourceView* pResolvedDepthStencilSRV);
	void modulateShadowBuffer(ID3D11RenderTargetView* pOutputRTV);
	void displayShadowMaps(ID3D11RenderTargetView* pOutputRTV, UINT Width, UINT Height);
	void displayMapFrustums(ID3D11RenderTargetView* pOutputRTV, ID3D11DepthStencilView* pDSV);
	void displayShadowBuffer(ID3D11RenderTargetView* pOutputRTV);
	void toggleDisplayCascades(bool bToggle);


	void drawUI();

private:
	void reloadBuffers();
	void ReleaseResources();

		
	GFSDK_ShadowLib_Context*			m_shadowLibContext;

	GFSDK_ShadowLib_ShaderResourceView	m_shadowBufferSRV;
	
	GFSDK_ShadowLib_Map*			m_shadowMapHandle;
	GFSDK_ShadowLib_MapDesc			m_SMDesc;
	GFSDK_ShadowLib_BufferDesc		m_SBDesc;
	GFSDK_ShadowLib_MapRenderParams	m_SMRenderParams;
	
	GFSDK_ShadowLib_Buffer*				m_shadowBufferHandle;
	GFSDK_ShadowLib_BufferRenderParams	m_SBRenderParams;

	GFSDK_ShadowLib_TempResources		m_tempResources;
	GFSDK_ShadowLib_Texture2D			m_downsampledShadowMap;
	
	CFirstPersonCamera* m_camera;

	// params
	bool m_PCSSEnabled;
	float m_lightSize;
	DirectX::XMFLOAT3 m_lightPos;
	DirectX::XMFLOAT3 m_lightLookAt;
	DirectX::XMFLOAT3 m_shadowColor;
	GFSDK_ShadowLib_PCSSPenumbraParams m_PCSSParams;
	float m_softShadowTestScale;

	gfsdk_float3 m_worldSpaceBBox0;
	gfsdk_float3 m_worldSpaceBBox1;

};


#endif