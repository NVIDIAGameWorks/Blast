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


#include "RendererHBAO.h"
#include "Renderer.h"
#include "imgui.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Renderer HBAO (wrapper for hbao+)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


RendererHBAO::RendererHBAO()
{
    m_SSAOContext = NULL;

    // default parameters
    m_SSAOParameters.Radius = 2.0f;
}


RendererHBAO::~RendererHBAO()
{
    releaseResources();
}


void RendererHBAO::createResources(ID3D11Device *pd3dDevice)
{
    GFSDK_SSAO_Status status;
    status = GFSDK_SSAO_CreateContext_D3D11(pd3dDevice, &m_SSAOContext, nullptr);
    assert(status == GFSDK_SSAO_OK);
}


void RendererHBAO::releaseResources()
{
    if (m_SSAOContext != NULL)
    {
        m_SSAOContext->Release();
    }
}


void RendererHBAO::renderAO(ID3D11DeviceContext *pd3dDeviceContext, ID3D11RenderTargetView* pRTV, ID3D11ShaderResourceView* pDepthSRV, DirectX::XMMATRIX& projMatrix)
{
    GFSDK_SSAO_InputData_D3D11 InputData;
    InputData.DepthData.pFullResDepthTextureSRV = pDepthSRV;
    InputData.DepthData.DepthTextureType = GFSDK_SSAO_HARDWARE_DEPTHS;
    InputData.DepthData.MetersToViewSpaceUnits = 1.0f;
    InputData.DepthData.ProjectionMatrix.Data = GFSDK_SSAO_Float4x4(reinterpret_cast<const GFSDK_SSAO_FLOAT*>(&(projMatrix.r[0])));
    InputData.DepthData.ProjectionMatrix.Layout = GFSDK_SSAO_ROW_MAJOR_ORDER;

    GFSDK_SSAO_Output_D3D11 Output;
    Output.pRenderTargetView = pRTV;// m_pSceneRTs->ColorRTV;
    Output.Blend.Mode = GFSDK_SSAO_MULTIPLY_RGB;

    m_SSAOContext->RenderAO(pd3dDeviceContext, InputData, m_SSAOParameters, Output);
}


void RendererHBAO::drawUI()
{
    ImGui::DragFloat("Radius", &(m_SSAOParameters.Radius), 0.05f, 0.0f, 100.0f);
    ImGui::DragFloat("Bias", &(m_SSAOParameters.Bias), 0.01f, 0.0f, 0.5f);
    ImGui::DragFloat("NearAO", &(m_SSAOParameters.NearAO), 0.01f, 1.0f, 4.0f);
    ImGui::DragFloat("FarAO", &(m_SSAOParameters.FarAO), 0.01, 1.0f, 4.0f);
    ImGui::DragFloat("PowerExponent", &(m_SSAOParameters.PowerExponent), 0.01f, 1.0f, 8.0f);
    ImGui::Checkbox("ForegroundAO Enabled", (bool*)&(m_SSAOParameters.ForegroundAO.Enable));
    ImGui::DragFloat("ForegroundAO ViewDepth", &(m_SSAOParameters.ForegroundAO.ForegroundViewDepth), 0.01f, 0.0f, 100.0f);
    ImGui::Checkbox("BackgroundAO Enabled", (bool*)&(m_SSAOParameters.BackgroundAO.Enable));
    ImGui::DragFloat("BackgroundAO ViewDepth", &(m_SSAOParameters.BackgroundAO.BackgroundViewDepth), 0.01f, 0.0f, 100.0f);
}