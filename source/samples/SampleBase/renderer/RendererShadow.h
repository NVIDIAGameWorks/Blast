// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2008-2022 NVIDIA Corporation. All rights reserved.


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

        
    GFSDK_ShadowLib_Context*            m_shadowLibContext;

    GFSDK_ShadowLib_ShaderResourceView  m_shadowBufferSRV;
    
    GFSDK_ShadowLib_Map*            m_shadowMapHandle;
    GFSDK_ShadowLib_MapDesc         m_SMDesc;
    GFSDK_ShadowLib_BufferDesc      m_SBDesc;
    GFSDK_ShadowLib_MapRenderParams m_SMRenderParams;
    
    GFSDK_ShadowLib_Buffer*             m_shadowBufferHandle;
    GFSDK_ShadowLib_BufferRenderParams  m_SBRenderParams;

    GFSDK_ShadowLib_TempResources       m_tempResources;
    GFSDK_ShadowLib_Texture2D           m_downsampledShadowMap;
    
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