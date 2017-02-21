/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/


#include "RendererShadow.h"

#include "XInput.h"
#include "DXUTMisc.h"
#include "DXUTCamera.h"
#include "Renderer.h"
#include "UIHelpers.h"

#define CASCADES 1

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//										Renderer Shadows (wrapper for shadow_lib)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const float DEFAULT_LIGHT_SIZE = 3.0f;
const DirectX::XMFLOAT3 DEFAULT_LIGHT_POS(-25, 25, 25);
const DirectX::XMFLOAT3 DEFAULT_LIGHT_LOOK_AT(0, 0, 0);
const DirectX::XMFLOAT3 DEFAULT_SHADOW_COLOR(0.25f, 0.25f, 0.25f);

RendererShadow::RendererShadow()
{
	m_shadowLibContext = NULL;

	m_PCSSEnabled = false;
	m_lightSize = DEFAULT_LIGHT_SIZE;
	m_lightPos = DEFAULT_LIGHT_POS;
	m_lightLookAt = DEFAULT_LIGHT_LOOK_AT;
	m_shadowColor = DEFAULT_SHADOW_COLOR;

	m_worldSpaceBBox0.x = m_worldSpaceBBox0.y = m_worldSpaceBBox0.z = -100;
	m_worldSpaceBBox1.x = m_worldSpaceBBox1.y = m_worldSpaceBBox1.z = 100;

	// Penumbra params
	m_PCSSParams.fMaxThreshold = 80.0f;
	m_PCSSParams.fMaxClamp = 40.0f;
	m_PCSSParams.fMinSizePercent = 3.0f;
	m_PCSSParams.fMinWeightExponent = 5.0f;
	m_PCSSParams.fMinWeightThresholdPercent = 20.0f;

	m_softShadowTestScale = 0.002f;

	memset(&m_shadowBufferSRV, 0, sizeof(m_shadowBufferSRV));

	m_shadowMapHandle = NULL;
	m_shadowBufferHandle = NULL;
}


RendererShadow::~RendererShadow()
{
	ReleaseResources();
}


void RendererShadow::createResources(ID3D11Device *pd3dDevice, ID3D11DeviceContext* context, CFirstPersonCamera* camera)
{
	m_camera = camera;

#if !CASCADES
	uint32_t shadowMapScale = 5;
	uint32_t shadowMapWidth = 1024;
	uint32_t shadowMapHeight = 1024;

	// SM Desc
	m_SMDesc.eViewType = GFSDK_ShadowLib_ViewType_Single;
	m_SMDesc.eMapType = GFSDK_ShadowLib_MapType_Texture;
#else

	uint32_t shadowMapScale = 5;
	uint32_t shadowMapWidth = 1024;
	uint32_t shadowMapHeight = 1024;

	// SM Desc
	m_SMDesc.eViewType = GFSDK_ShadowLib_ViewType_Cascades_2;
	m_SMDesc.eMapType = GFSDK_ShadowLib_MapType_TextureArray;
#endif

	m_SMDesc.uResolutionWidth = shadowMapWidth * shadowMapScale;
	m_SMDesc.uResolutionHeight = shadowMapHeight * shadowMapScale;
	m_SMDesc.uArraySize = m_SMDesc.eViewType;

	for (int j = 0; j < GFSDK_ShadowLib_ViewType_Cascades_4; j++)
	{
		m_SMDesc.ViewLocation[j].uMapID = j;
		m_SMDesc.ViewLocation[j].v2Origin.x = 0;
		m_SMDesc.ViewLocation[j].v2Origin.y = 0;
		m_SMDesc.ViewLocation[j].v2Dimension.x = shadowMapWidth * shadowMapScale;
		m_SMDesc.ViewLocation[j].v2Dimension.y = shadowMapHeight * shadowMapScale;
	}


	// SM Render Params
	m_SMRenderParams.iDepthBias = 1000;
	m_SMRenderParams.fSlopeScaledDepthBias = 8;

	// SB Render Params
	m_SBRenderParams.eTechniqueType = GFSDK_ShadowLib_TechniqueType_PCSS;
	m_SBRenderParams.eQualityType = GFSDK_ShadowLib_QualityType_High;

	// DLL version
	GFSDK_ShadowLib_Version DLLVersion;
	GFSDK_ShadowLib_Status retCode = GFSDK_ShadowLib_GetDLLVersion(&DLLVersion);

	// Header version
	GFSDK_ShadowLib_Version headerVersion;
	headerVersion.uMajor = GFSDK_SHADOWLIB_MAJOR_VERSION;
	headerVersion.uMinor = GFSDK_SHADOWLIB_MINOR_VERSION;

	// Do they match?
	if (DLLVersion.uMajor == headerVersion.uMajor && DLLVersion.uMinor == headerVersion.uMinor)
	{
		GFSDK_ShadowLib_DeviceContext deviceAndContext;
		deviceAndContext.pD3DDevice = pd3dDevice;
		deviceAndContext.pDeviceContext = context;

		retCode = GFSDK_ShadowLib_Create(&headerVersion, &m_shadowLibContext, &deviceAndContext, NULL);

		if (retCode != GFSDK_ShadowLib_Status_Ok) assert(false);
	}
	else
	{
		assert(false);
	}
}


void RendererShadow::ReleaseResources()
{
	SAFE_RELEASE(m_downsampledShadowMap.pTexture);
	SAFE_RELEASE(m_downsampledShadowMap.pSRV);
	SAFE_RELEASE(m_downsampledShadowMap.pRTV);

	if (m_shadowLibContext != NULL)
	{
		m_shadowLibContext->Destroy();
		m_shadowLibContext = NULL;
	}
}


void RendererShadow::setScreenResolution(float FovyRad, UINT Width, UINT Height, UINT uSampleCount, ID3D11DepthStencilView* pReadOnlyDSV)
{
	changeShadowSettings(Width, Height, uSampleCount, pReadOnlyDSV);
}


void RendererShadow::changeShadowSettings(UINT Width, UINT Height, UINT uSampleCount, ID3D11DepthStencilView* pReadOnlyDSV)
{
	m_SBDesc.uResolutionWidth = Width;
	m_SBDesc.uResolutionHeight = Height;
	m_SBDesc.uSampleCount = uSampleCount;
	m_SBDesc.ReadOnlyDSV.pDSV = pReadOnlyDSV;

	reloadBuffers();
}

void RendererShadow::reloadBuffers()
{
	{
		m_shadowLibContext->RemoveMap(&m_shadowMapHandle);

		if (m_SMDesc.eMapType == GFSDK_ShadowLib_MapType_Texture &&
			m_SMDesc.eViewType == GFSDK_ShadowLib_ViewType_Single && 
			m_SBRenderParams.eTechniqueType == GFSDK_ShadowLib_TechniqueType_PCSS)
		{
			m_SMDesc.bDownsample = true;
		}

		m_shadowLibContext->AddMap(&m_SMDesc, &m_shadowMapHandle);
	}

	if (m_SMDesc.eMapType == GFSDK_ShadowLib_MapType_Texture && m_SMDesc.eViewType == GFSDK_ShadowLib_ViewType_Single)
	{
		m_downsampledShadowMap.uWidth = m_SMDesc.uResolutionWidth >> 1;
		m_downsampledShadowMap.uHeight = m_SMDesc.uResolutionHeight >> 1;
		m_downsampledShadowMap.uSampleCount = 1;
		m_downsampledShadowMap.Format = DXGI_FORMAT_R32_FLOAT;
		SAFE_RELEASE(m_downsampledShadowMap.pTexture);
		SAFE_RELEASE(m_downsampledShadowMap.pSRV);
		SAFE_RELEASE(m_downsampledShadowMap.pRTV);
		m_shadowLibContext->DevModeCreateTexture2D(&m_downsampledShadowMap);
	}

	m_shadowLibContext->RemoveBuffer(&m_shadowBufferHandle);
	m_shadowLibContext->AddBuffer(&m_SBDesc, &m_shadowBufferHandle);
}



//--------------------------------------------------------------------------------------
// Data passed to the shadow map render function 
//--------------------------------------------------------------------------------------
struct ShadowMapRenderFunctionParams
{
	Renderer* renderer;
};
static ShadowMapRenderFunctionParams s_RenderParams;

//--------------------------------------------------------------------------------------
// Shadow map render function
//--------------------------------------------------------------------------------------
static void ShadowMapRenderFunction(void* pParams, gfsdk_float4x4* pViewProj)
{
	ShadowMapRenderFunctionParams* pRP = (ShadowMapRenderFunctionParams*)pParams;

	DirectX::XMMATRIX viewProjection;
	memcpy(&viewProjection, &pViewProj->_11, sizeof(gfsdk_float4x4));

	pRP->renderer->renderDepthOnly(&viewProjection);
}

void RendererShadow::renderShadowMaps(Renderer* renderer)
{
	// select technique
	GFSDK_ShadowLib_TechniqueType technique = m_SBRenderParams.eTechniqueType;
	m_SBRenderParams.eTechniqueType = m_PCSSEnabled ? GFSDK_ShadowLib_TechniqueType_PCSS : GFSDK_ShadowLib_TechniqueType_PCF;
	if (technique != m_SBRenderParams.eTechniqueType)
		reloadBuffers();


	DirectX::XMMATRIX viewMatrix = m_camera->GetViewMatrix();
	DirectX::XMMATRIX projMatrix = m_camera->GetProjMatrix();

	memcpy(&m_SMRenderParams.m4x4EyeViewMatrix, &viewMatrix.r[0], sizeof(gfsdk_float4x4));
	memcpy(&m_SMRenderParams.m4x4EyeProjectionMatrix, &projMatrix.r[0], sizeof(gfsdk_float4x4));

	// TODO: (better world space bbox needed)
	m_SMRenderParams.v3WorldSpaceBBox[0] = m_worldSpaceBBox0;
	m_SMRenderParams.v3WorldSpaceBBox[1] = m_worldSpaceBBox1;

	m_SMRenderParams.LightDesc.eLightType = GFSDK_ShadowLib_LightType_Directional;
	memcpy(&m_SMRenderParams.LightDesc.v3LightPos, &m_lightPos.x, sizeof(gfsdk_float3));
	memcpy(&m_SMRenderParams.LightDesc.v3LightLookAt, &m_lightLookAt.x, sizeof(gfsdk_float3));
	m_SMRenderParams.LightDesc.fLightSize = m_lightSize;
	m_SMRenderParams.LightDesc.bLightFalloff = false;

	// Scene specific setup for the shadow map phase that follows
	s_RenderParams.renderer = renderer;
	m_SMRenderParams.fnpDrawFunction = GFSDK_ShadowLib_FunctionPointer(ShadowMapRenderFunction);
	m_SMRenderParams.pDrawFunctionParams = &s_RenderParams;

	// render shadow map
	m_shadowLibContext->RenderMap(m_shadowMapHandle, &m_SMRenderParams);
}


void RendererShadow::renderShadowBuffer(ID3D11ShaderResourceView* pDepthStencilSRV, ID3D11ShaderResourceView* pResolvedDepthStencilSRV)
{
	if (m_SBRenderParams.eTechniqueType == GFSDK_ShadowLib_TechniqueType_PCSS &&
		m_SMDesc.eMapType == GFSDK_ShadowLib_MapType_Texture &&
		m_SMDesc.eViewType == GFSDK_ShadowLib_ViewType_Single)
	{
		m_tempResources.pDownsampledShadowMap = &m_downsampledShadowMap;
		m_shadowLibContext->SetTempResources(&m_tempResources);
	}

	m_SBRenderParams.PCSSPenumbraParams = m_PCSSParams;
	m_SBRenderParams.fSoftShadowTestScale = m_softShadowTestScale;

	m_shadowLibContext->ClearBuffer(m_shadowBufferHandle);

	m_SBRenderParams.DepthBufferDesc.DepthStencilSRV.pSRV = pDepthStencilSRV;

	m_shadowLibContext->RenderBuffer(m_shadowMapHandle, m_shadowBufferHandle, &m_SBRenderParams);

	m_shadowLibContext->FinalizeBuffer(m_shadowBufferHandle, &m_shadowBufferSRV);
}


void RendererShadow::modulateShadowBuffer(ID3D11RenderTargetView* pOutputRTV)
{
	GFSDK_ShadowLib_RenderTargetView ColorRTV;
	ColorRTV.pRTV = pOutputRTV;

	gfsdk_float3 v3ShadowColor = { m_shadowColor.x, m_shadowColor.y, m_shadowColor.z };
	m_shadowLibContext->ModulateBuffer(m_shadowBufferHandle, &ColorRTV, v3ShadowColor, GFSDK_ShadowLib_ModulateBufferMask_RGB);
}


void RendererShadow::displayShadowMaps(ID3D11RenderTargetView* pOutputRTV, UINT Width, UINT Height)
{
	GFSDK_ShadowLib_RenderTargetView ColorRTV;
	ColorRTV.pRTV = pOutputRTV;

	float fMapResW = (float)m_SMDesc.uResolutionWidth;
	float fMapResH = (float)m_SMDesc.uResolutionHeight;

	float fWidthScale = Width / ((float)m_SMDesc.uArraySize * fMapResW);
	fWidthScale = (fWidthScale > 1.0f) ? (1.0f) : (fWidthScale);

	float fOneFifth = (float)Height / (5.0f);
	float fHeightScale = fOneFifth / fMapResH;
	fHeightScale = (fHeightScale > 1.0f) ? (1.0f) : (fHeightScale);

	float fScale = (fHeightScale < fWidthScale) ? (fHeightScale) : (fWidthScale);

	fMapResW = floorf(fMapResW * fScale);
	fMapResH = floorf(fMapResH * fScale);

	for (unsigned int j = 0; j < (unsigned int)m_SMDesc.uArraySize; j++)
	{
		m_shadowLibContext->DevModeDisplayMap(m_shadowBufferHandle,
			&ColorRTV,
			m_shadowMapHandle,
			j,
			j * (unsigned int)fMapResW + j,
			Height - (unsigned int)fMapResH,
			fScale);
	}
}


void RendererShadow::displayMapFrustums(ID3D11RenderTargetView* pOutputRTV, ID3D11DepthStencilView* pDSV)
{
	gfsdk_float3 v3Color;
	v3Color.x = 1.0f;
	v3Color.y = 0.0f;
	v3Color.z = 0.0f;

	GFSDK_ShadowLib_RenderTargetView ColorRTV;
	ColorRTV.pRTV = pOutputRTV;

	GFSDK_ShadowLib_DepthStencilView DSV;
	DSV.pDSV = pDSV;

	unsigned int NumViews;
	NumViews = m_SMDesc.eViewType;

	for (unsigned int j = 0; j < NumViews; j++)
	{
		switch (j)
		{
		case 0:
			v3Color.x = 1.0f;
			v3Color.y = 0.0f;
			v3Color.z = 0.0f;
			break;
		case 1:
			v3Color.x = 0.0f;
			v3Color.y = 1.0f;
			v3Color.z = 0.0f;
			break;
		case 2:
			v3Color.x = 0.0f;
			v3Color.y = 0.0f;
			v3Color.z = 1.0f;
			break;
		case 3:
			v3Color.x = 1.0f;
			v3Color.y = 1.0f;
			v3Color.z = 0.0f;
			break;
		}

		m_shadowLibContext->DevModeDisplayMapFrustum(m_shadowBufferHandle,
			&ColorRTV,
			&DSV,
			m_shadowMapHandle,
			j,
			v3Color);
	}
}


void RendererShadow::displayShadowBuffer(ID3D11RenderTargetView* pOutputRTV)
{
	gfsdk_float2 v2Scale;
	v2Scale.x = 1.0f;
	v2Scale.y = 1.0f;

	GFSDK_ShadowLib_RenderTargetView ColorRTV;
	ColorRTV.pRTV = pOutputRTV;

	m_shadowLibContext->DevModeDisplayBuffer(m_shadowBufferHandle,
		&ColorRTV,
		v2Scale,
		NULL);
}


void RendererShadow::toggleDisplayCascades(bool bToggle)
{
	m_shadowLibContext->DevModeToggleDebugCascadeShader(m_shadowBufferHandle,
		bToggle);
}


void RendererShadow::drawUI()
{
	ImGui::Checkbox("PCSS", &m_PCSSEnabled);
	ImGui::ColorEdit3("Shadow Color", &(m_shadowColor.x));
	ImGui::DragFloat("Light Size", &m_lightSize, 0.05f, 0.0f, 100.0f);
	ImGui::DragFloat3("Light Position", &(m_lightPos.x));
	ImGui_DragFloat3Dir("Light LookAt", &(m_lightLookAt.x));
	ImGui::DragFloat("SoftShadowTestScale", &(m_softShadowTestScale), 0.0001f, 0.0f, 10.0f);
	if (m_PCSSEnabled)
	{
		ImGui::DragFloat("PCSS: fMaxClamp", &(m_PCSSParams.fMaxClamp), 0.001f, 0.0f, 100.0f);
		ImGui::DragFloat("PCSS: fMaxThreshold", &(m_PCSSParams.fMaxThreshold), 0.001f, 0.0f, 100.0f);
		ImGui::DragFloat("PCSS: fMinSizePercent", &(m_PCSSParams.fMinSizePercent), 0.001f, 0.0f, 100.0f);
		ImGui::DragFloat("PCSS: fMinWeightExponent", &(m_PCSSParams.fMinWeightExponent), 0.001f, 0.0f, 100.0f);
		ImGui::DragFloat("PCSS: fMinWeightThresholdPercent", &(m_PCSSParams.fMinWeightThresholdPercent), 0.001f, 0.0f, 100.0f);
		ImGui::DragFloat("PCSS: fBlockerSearchDitherPercent", &(m_PCSSParams.fBlockerSearchDitherPercent), 0.001f, 0.0f, 100.0f);
		ImGui::DragFloat("PCSS: fFilterDitherPercent", &(m_PCSSParams.fFilterDitherPercent), 0.001f, 0.0f, 100.0f);
	}
}