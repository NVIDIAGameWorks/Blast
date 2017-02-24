/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef RENDERER_HBAO_H
#define RENDERER_HBAO_H

#include <DirectXMath.h>
#include "GFSDK_SSAO.h"


class Renderer;

class RendererHBAO
{
public:
	RendererHBAO();
	~RendererHBAO();

	void createResources(ID3D11Device *pd3dDevice);
	void renderAO(ID3D11DeviceContext *pd3dDeviceContext, ID3D11RenderTargetView* pRTV, ID3D11ShaderResourceView* pDepthSRV, DirectX::XMMATRIX& projMatrix);

	void drawUI();

private:
	void releaseResources();

	GFSDK_SSAO_Parameters m_SSAOParameters;

	GFSDK_SSAO_Context_D3D11* m_SSAOContext;
};


#endif