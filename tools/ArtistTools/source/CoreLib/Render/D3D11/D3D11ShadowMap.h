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

#include "d3d11.h"
#include "D3D11TextureResource.h"

class GPUShaderResource;
class D3D11TextureResource;

struct D3D11ShadowMap : public ShadowMap
{
	D3D11_VIEWPORT				m_viewport;

	ID3D11Texture2D*			m_pShadowTexture;
	ID3D11RenderTargetView*		m_pShadowRTV;
	ID3D11ShaderResourceView*	m_pShadowSRV;

	ID3D11Texture2D*			m_pDepthTexture;
	ID3D11DepthStencilView*		m_pDepthDSV;
	ID3D11ShaderResourceView*	m_pDepthSRV;

	D3D11TextureResource		m_shadowResource;
	
public:
	D3D11ShadowMap(int resolution );
	~D3D11ShadowMap();

	void Release();
	void BeginRendering(float clearDepth);
	void EndRendering();

	GPUShaderResource* GetShadowSRV();

protected:

	bool isValid();

	ID3D11RenderTargetView*	m_pPreviousRTV;
	ID3D11DepthStencilView*	m_pPreviousDSV;
	UINT					m_numPreviousViewports;
	D3D11_VIEWPORT			m_previousViewport[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
};

