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
#include "Light.h"

#include "GlobalSettings.h"
#include "ShadowMap.h"
#include "SimpleRenderable.h"
#include "MeshShaderParam.h"

#include "RenderInterface.h"

//////////////////////////////////////////////////////////////////////////////
inline int GetShadowMapResolution(int index)
{
	switch (index)
	{
	case 0: // default
		return 2048;
		break;
	case 1:
		return 4096;
		break;
	case 2:
		return 1024;
		break;
	case 3:
		return 512;
		break;
	case 4:
		return 8192;
		break;
	case 5:
		return 16384;
		break;
	}

	return 1024;
}

namespace
{
	std::vector<Light>			g_Lights;
	static bool					g_LinkLightOption = false;
	std::string					m_envTextureFilePath;
	GPUShaderResource*			m_pEnvTextureSRV;
}

//////////////////////////////////////////////////////////////////////////////
Light::Light()	
{
	m_enable		= true;
	m_useShadows	= false;
	m_visualize		= false;
	m_isEnvLight	= false;
	m_useEnvMap		= false;

	m_selected		= false;

	m_color			= gfsdk_makeFloat3(1.0f, 1.0f, 1.0f);
	m_intensity		= 1.0f;

	m_pShadowMap = 0;
}

//////////////////////////////////////////////////////////////////////////////
Light::~Light()
{
	if (m_pShadowMap)
	{
		delete m_pShadowMap;
		m_pShadowMap = 0;
	}
}

//////////////////////////////////////////////////////////////////////////////
bool Light::UseLHS() const { 
	return m_lightCamera.UseLHS(); 
}

//////////////////////////////////////////////////////////////////////////////
GPUShaderResource* Light::GetShadowSRV()
{
	if (!m_pShadowMap)
		return 0;

	return m_pShadowMap->GetShadowSRV();
}

//////////////////////////////////////////////////////////////////////////////
void Light::SetShadowMapResolution(int index)
{
	if (m_shadowMapResolutionIndex == index)
		return;

	m_lightMapResolution = GetShadowMapResolution(index);

	if (m_pShadowMap)
	{
		m_pShadowMap->Release();
		delete m_pShadowMap;
	}

	m_pShadowMap = ShadowMap::Create(m_lightMapResolution);

	float minZ = 1.0f;
	float maxZ = 10000.0f;	// should calculate dynamically

	m_lightCamera.Ortho(m_lightMapResolution, m_lightMapResolution, minZ, maxZ);

	m_shadowMapResolutionIndex = index;

	FitBounds();
}

//////////////////////////////////////////////////////////////////////////////
void Light::Init()	
{
	m_shadowMapResolutionIndex = 2;
	m_lightMapResolution = GetShadowMapResolution(m_shadowMapResolutionIndex);

	m_pShadowMap = ShadowMap::Create(m_lightMapResolution);
}

//////////////////////////////////////////////////////////////////////////////
void Light::Release()
{
	if (m_pShadowMap)
	{
		delete m_pShadowMap;
		m_pShadowMap = 0;
	}
}

//////////////////////////////////////////////////////////////////////////////
void Light::InitCamera(const atcore_float3& from, const atcore_float3& lookAt)
{
	m_lightCamera.Init(false, false);

//	atcore_float3 up = gfsdk_makeFloat3(0.0f, 1.0f, 0.0f);
	atcore_float3 up = gfsdk_makeFloat3(0.0f, 0.0f, 1.0f);

	m_lightCamera.LookAt(from, lookAt, up);

	float minZ = 1.0f;
	float maxZ = 10000.0f;	// should calculate dynamically

	m_lightCamera.Ortho(m_lightMapResolution, m_lightMapResolution, minZ, maxZ);
}

//////////////////////////////////////////////////////////////////////////////
atcore_float3 
Light::getLightDirection() const
{
	return (atcore_float3&)m_lightCamera.GetViewDirection();
}

//////////////////////////////////////////////////////////////////////////////
void Light::SetBounds(atcore_float3* bbCenter, atcore_float3* bbExtents)
{
	if (bbCenter)
		m_bbCenter = *bbCenter;
	if (bbExtents)
		m_bbExtent = *bbExtents;
}

//////////////////////////////////////////////////////////////////////////////
void Light::FitBounds(bool updateCenter)
{
	////////////////////////////////////////////////////////
	atcore_float3 bbCenter = m_bbCenter;
	atcore_float3 bbExtents =m_bbExtent;

	////////////////////////////////////////////////////////////////////////
	if (updateCenter)
	{
		atcore_float3 lightPos		= m_lightCamera.GetEye();
		atcore_float3 lightAt		= m_lightCamera.GetAt();

		atcore_float3 disp = bbCenter - lightAt;

		lightAt		= lightAt + disp;
		lightPos	= lightPos + disp;

		m_lightCamera.SetEye(lightPos);
		m_lightCamera.SetAt(lightAt);
		m_lightCamera.BuildViewMatrix();
	}

	////////////////////////////////////////////////////////////////////////
	float size = bbExtents.x;
	
	size = max(size, bbExtents.y);
	size = max(size, bbExtents.z);

	atcore_float4x4 view = m_lightCamera.GetViewMatrix();

	atcore_float3 c = gfsdk_transformCoord(view, bbCenter);

	size *= 3.0f;

	float minZ = c.z - size;
	float maxZ = c.z + size;

	float orthoW = size;
	float orthoH = size;

	if (m_lightCamera.UseLHS())
		m_lightCamera.Ortho(orthoW, orthoH, minZ, maxZ);
	else // rhs camera flips Z
		m_lightCamera.Ortho(orthoW, orthoH, -maxZ, -minZ);

}

//////////////////////////////////////////////////////////////////////////////
void Light::resetUpDir(bool zup)
{
	m_lightCamera.ResetUpDir(zup);
}

//////////////////////////////////////////////////////////////////////////////
void Light::resetLhs(bool lhs)
{
	m_lightCamera.ResetLhs(lhs);
	FitBounds();
}

//////////////////////////////////////////////////////////////////////////////
void Light::Orbit(const atcore_float2& delta)
{
	m_lightCamera.OrbitLight(gfsdk_makeFloat2(-delta.x, delta.y));

	FitBounds();
}

//////////////////////////////////////////////////////////////////////////////
void Light::Pan(const atcore_float2& delta, const atcore_float3& axisX, const atcore_float3& axisY)
{
	atcore_float3 lightPos	= m_lightCamera.GetEye();
	atcore_float3 lightAt	= m_lightCamera.GetAt();
	atcore_float3 lightDir 	= m_lightCamera.GetZAxis();

	float depth = GetDistance();

	atcore_float2 newDelta = depth * delta;

	atcore_float3 disp = -1.0f * axisY * newDelta.y + axisX * newDelta.x;

	lightAt = lightAt + disp;
	lightPos = lightPos + disp;

	m_lightCamera.SetEye(lightPos);
	m_lightCamera.SetAt(lightAt);
	m_lightCamera.BuildViewMatrix();

	FitBounds();
}

//////////////////////////////////////////////////////////////////////////////
float Light::GetDistance() const
{
	return m_lightCamera.GetLookDistance();
}

//////////////////////////////////////////////////////////////////////////////
bool Light::SetDistance(float newdistance)
{
	atcore_float3 lightPos		= m_lightCamera.GetEye();
	atcore_float3 lightAt		= m_lightCamera.GetAt();
	atcore_float3 lightDir 		= m_lightCamera.GetZAxis();

	lightPos = lightAt + newdistance * lightDir;

	m_lightCamera.SetEye(lightPos);
	m_lightCamera.BuildViewMatrix();

	FitBounds();

	return true;
}
//////////////////////////////////////////////////////////////////////////////
void Light::BeginShadowMapRendering()
{
	if (!m_pShadowMap) return;

	float clearDepth = m_lightCamera.UseLHS() ? FLT_MAX : -FLT_MAX;
	m_pShadowMap->BeginRendering(clearDepth);
}

//////////////////////////////////////////////////////////////////////////////
void Light::EndShadowMapRendering()
{
	if (!m_pShadowMap) return;

	m_pShadowMap->EndRendering();
}

//////////////////////////////////////////////////////////////////////////////
atcore_float4x4 Light::GetViewMatrix() const
{
	return m_lightCamera.GetViewMatrix();
}

//////////////////////////////////////////////////////////////////////////////
atcore_float4x4 Light::GetProjectionMatrix() const
{
	return m_lightCamera.GetProjectionMatrix();
}

//////////////////////////////////////////////////////////////////////////////
atcore_float4x4 Light::GetLightMatrix() const
{
	atcore_float4x4 view = m_lightCamera.GetViewMatrix();
	atcore_float4x4 projection = m_lightCamera.GetProjectionMatrix();

	float mClip2Tex[] = { 
				0.5,    0,    0,   0,
				0,	   -0.5,  0,   0,
				0,     0,     1,   0,
				0.5,   0.5,   0,   1 
			};
	atcore_float4x4 clip2Tex = (atcore_float4x4&)mClip2Tex;

	atcore_float4x4 viewProjection = view * projection;

	atcore_float4x4 lightMatrix = viewProjection * clip2Tex;

	return lightMatrix;
}

///////////////////////////////////////////////////////////////////////////////
void Light::FillLightShaderParam(LightShaderParam &param)
{
	const std::vector<Light> &lights = g_Lights;

	for (int i = 0; i < lights.size(); i++)
	{
		if (i >= 4)
			break;

		LightParam& lparam = param.m_lightParam[i];
		memset(&lparam, 0, sizeof(LightParam));

		const Light& light= lights[i];

		lparam.m_enable		= light.m_enable;
		lparam.m_useShadows	= light.m_useShadows;

		lparam.m_dir		= light.getLightDirection();
		lparam.m_intensity	= light.m_intensity;
		lparam.m_color      = light.m_color;
		lparam.m_isEnvLight = light.m_isEnvLight;
		lparam.m_useEnvMap	= light.m_isEnvLight && (m_pEnvTextureSRV != 0);

		float sceneUnit = GlobalSettings::Inst().getSceneUnitInCentimeters();
		lparam.m_depthBias  = (sceneUnit > 0.0f) ? 1.0f / sceneUnit : 1.0f;

		if (light.UseLHS())
		{
			lparam.m_depthBias *= -1.0f;
			lparam.m_depthGain = -1.0f;
			lparam.m_lhs = true;
		}
		else
		{
			lparam.m_depthGain = 1.0f;
			lparam.m_lhs = false;
		}

		lparam.m_viewMatrix = light.GetViewMatrix();
		lparam.m_lightMatrix = light.GetLightMatrix();
	}
}

//////////////////////////////////////////////////////////////////////////////
void Light::draw(Camera* pCamera)
{
	atcore_float3 lightPos	= m_lightCamera.GetEye();
	atcore_float3 lightAt	= m_lightCamera.GetAt();

	atcore_float4 color = gfsdk_makeFloat4(m_color.x, m_color.y, m_color.z, 1);
	if (m_enable == false)
		color = gfsdk_makeFloat4(0.5, 0.5, 0.5, 1);

	// draw light shape
	atcore_float4x4 lightMat = gfsdk_transpose(m_lightCamera.GetViewMatrix());
		gfsdk_setPosition(lightMat, lightPos);

	SimpleShaderParam param;
	{
		param.world		= lightMat;
		param.view		= pCamera->GetViewMatrix();
		param.projection = pCamera->GetProjectionMatrix();
		param.useVertexColor = false;
		param.color = color;
	}

	RenderInterface::CopyShaderParam(RenderInterface::SHADER_TYPE_SIMPLE_COLOR,
		(void*)&param, sizeof(SimpleShaderParam) );

	SimpleRenderable::Draw(SimpleRenderable::LIGHT);
	
	// draw light ray
	gfsdk_makeIdentity(param.world);
	RenderInterface::CopyShaderParam(RenderInterface::SHADER_TYPE_SIMPLE_COLOR,
		(void*)&param, sizeof(SimpleShaderParam) );

	SimpleRenderable::DrawLine(lightAt, lightPos);
}

//////////////////////////////////////////////////////////////////////////////
bool Light::loadParameters(NvParameterized::Handle& handle)
{
#ifndef NV_ARTISTTOOLS
	NvParameterized::NvParameters* params = static_cast<NvParameterized::NvParameters*>(handle.getInterface());
	size_t offset = 0;
	nvidia::parameterized::HairProjectParametersNS::Light_Type* param = nullptr;
	params->getVarPtr(handle, (void*&)param, offset);

	m_enable = param->enable;
	m_useShadows = param->useShadows;
	m_visualize = param->visualize;
	m_intensity = param->intensity;

	memcpy(&m_color, &param->color, sizeof(atcore_float3));

	atcore_float3 axisX, axisY, axisZ, lightPos;
	memcpy(&axisX, &param->lightAxisX, sizeof(atcore_float3));
	memcpy(&axisY, &param->lightAxisY, sizeof(atcore_float3));
	memcpy(&axisZ, &param->lightAxisZ, sizeof(atcore_float3));
	memcpy(&lightPos, &param->lightPos, sizeof(atcore_float3));
	
	this->SetShadowMapResolution(param->shadowMapResolution);

	m_lightCamera.SetEye(lightPos);
	m_lightCamera.SetViewMatrix(axisX, axisY, axisZ);
	m_lightCamera.BuildViewMatrix();
#else
	CoreLib::Inst()->Light_loadParameters(handle, this);
#endif // NV_ARTISTTOOLS

	return true;
}

//////////////////////////////////////////////////////////////////////////////
bool Light::saveParameters(NvParameterized::Handle& handle)
{
#ifndef NV_ARTISTTOOLS
	NvParameterized::NvParameters* params = static_cast<NvParameterized::NvParameters*>(handle.getInterface());
	size_t offset = 0;
	nvidia::parameterized::HairProjectParametersNS::Light_Type* param = nullptr;
	params->getVarPtr(handle, (void*&)param, offset);

	param->enable = m_enable;
	param->useShadows = m_useShadows;
	param->visualize = m_visualize;
	param->intensity = m_intensity;

	param->shadowMapResolution = m_shadowMapResolutionIndex;

	memcpy(&param->color, &m_color, sizeof(atcore_float3));

	{
		atcore_float3 axisX = m_lightCamera.GetXAxis();
		atcore_float3 axisY = m_lightCamera.GetYAxis();
		atcore_float3 axisZ = m_lightCamera.GetZAxis();
		atcore_float3 lightPos = m_lightCamera.GetEye();

		memcpy(&param->lightAxisX, &axisX, sizeof(atcore_float3));
		memcpy(&param->lightAxisY, &axisY, sizeof(atcore_float3));
		memcpy(&param->lightAxisZ, &axisZ, sizeof(atcore_float3));
		memcpy(&param->lightPos, &lightPos, sizeof(atcore_float3));
	}
#else
	CoreLib::Inst()->Light_saveParameters(handle, this);
#endif // NV_ARTISTTOOLS

	return true;
}

//////////////////////////////////////////////////////////////////////////////
bool Light::SetEnvTextureFromFilePath(const char* textureFilePath)
{
	Light* pLight = Light::GetFirstSelectedLight();
	if (!pLight || !pLight->m_isEnvLight)
		return false;

	m_envTextureFilePath = (textureFilePath) ? textureFilePath : "";

	SAFE_RELEASE(m_pEnvTextureSRV);
	if ((!textureFilePath) && (strlen(textureFilePath) > 0))
		m_pEnvTextureSRV = RenderInterface::CreateTextureResource(textureFilePath);

	return true;
}

//////////////////////////////////////////////////////////////////////////////
const std::string& Light::GetEnvTextureFilePath()
{
	return m_envTextureFilePath;
}

//////////////////////////////////////////////////////////////////////////////
GPUShaderResource* Light::GetEnvTextureSRV()
{
	return m_pEnvTextureSRV;
}

//////////////////////////////////////////////////////////////////////////////
GPUShaderResource* Light::GetShadowSRV(int i)
{
	if ((i >= 3) || (i < 0))
		return 0;

	return g_Lights[i].GetShadowSRV();
}

bool Light::GetLinkLightOption()
{
	return g_LinkLightOption;
}

void Light::SetLinkLightOption(bool val)
{
	g_LinkLightOption = val;
}

//////////////////////////////////////////////////////////////////////////////
std::vector<Light>& Light::GetDefaultLights()
{
	return g_Lights;
}

//////////////////////////////////////////////////////////////////////////////
Light* Light::GetFirstSelectedLight()
{
	for (int i = 0; i < g_Lights.size(); i++)
	{
		if (g_Lights[i].m_selected)
			return &g_Lights[i];
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////
Light* Light::GetLight(int index)
{
	if (index < 0)
		return 0;
	if (index >= g_Lights.size())
		return 0;

	return &g_Lights[index];
}

//////////////////////////////////////////////////////////////////////////////
void Light::Initialize()
{
	g_Lights.resize(4);

	Light& keyLight		= g_Lights[0];
	Light& fillLight	= g_Lights[1];
	Light& rimLight		= g_Lights[2];
	Light& envLight		= g_Lights[3];

	atcore_float3 lookAt = gfsdk_makeFloat3(0.0f, 0.0f, 0.0f);

	keyLight.m_selected = true;
	keyLight.m_enable = true;
	keyLight.m_color = gfsdk_makeFloat3(1, 1, 1);
	keyLight.m_useShadows = true;
	keyLight.m_isEnvLight = false;
	keyLight.InitCamera(gfsdk_makeFloat3(100.0f, 25.0f, 0.0f), lookAt);
	keyLight.m_name = "Key Light";

	fillLight.m_enable = false;
	fillLight.m_color = gfsdk_makeFloat3(0.5, 0.5, 0.5);
	fillLight.m_useShadows = false;
	fillLight.m_isEnvLight = false;
	fillLight.InitCamera(gfsdk_makeFloat3(-100.0f, 0.0f, 25.0f), lookAt);
	fillLight.m_name = "Fill Light";
	
	rimLight.m_enable = false;
	rimLight.m_color = gfsdk_makeFloat3(0.25, 0.25, 0.25);
	rimLight.m_useShadows = false;
	rimLight.m_isEnvLight = false;
	rimLight.InitCamera(gfsdk_makeFloat3(0.0f, 100.0f, -25.0f), lookAt);
	rimLight.m_name = "Rim Light";

	envLight.m_enable = false;
	envLight.m_color = gfsdk_makeFloat3(0.25, 0.25, 0.25);
	envLight.m_useShadows = false;
	envLight.m_isEnvLight = true;
	envLight.InitCamera(gfsdk_makeFloat3(0.0f, 0.0f, 0.0f), lookAt);
	envLight.m_name = "Env Light";

	for (int i = 0; i < g_Lights.size(); i++)
		g_Lights[i].Init();

}

//////////////////////////////////////////////////////////////////////////////
void Light::Shutdown()
{
	for (int i = 0; i < g_Lights.size(); i++)
		g_Lights[i].Release();

	g_Lights.clear();
}

//////////////////////////////////////////////////////////////////////////////
void Light::FitLightCameras(atcore_float3& center, atcore_float3& extent)
{
	for (int i = 0; i < g_Lights.size(); i++)
	{
		if (g_LinkLightOption && (i == FILL_LIGHT || i == RIM_LIGHT))
			continue;

		Light& light = g_Lights[i];
		light.SetBounds(&center, &extent);
		light.FitBounds(true);
	}
}

//////////////////////////////////////////////////////////////////////////////
void Light::ResetUpDir(bool zup)
{
	for (int i = 0; i < g_Lights.size(); i++)
	{
		Light& li = g_Lights[i];
		li.resetUpDir(zup);
	}
}

//////////////////////////////////////////////////////////////////////////////
void Light::ResetLhs(bool lhs)
{
	for (int i = 0; i < g_Lights.size(); i++)
	{
		Light& li = g_Lights[i];
		li.resetLhs(lhs);
	}
}

//////////////////////////////////////////////////////////////////////////////
void Light::DrawLights(Camera* pCamera)
{
	for (int i = 0; i < g_Lights.size(); i++)
	{
		Light& light = g_Lights[i];
		if (light.m_visualize)
			light.draw(pCamera);
	}
}

//////////////////////////////////////////////////////////////////////////////
void Light::RenderShadowMap()
{
	Light* pLight = Light::GetFirstSelectedLight();
	if (pLight)
	{
		float zNear = pLight->m_lightCamera.GetZNear();
		float zFar = pLight->m_lightCamera.GetZFar();

		if (pLight->UseLHS())
			RenderInterface::RenderShadowMap(pLight->GetShadowSRV(), zNear, zFar);
		else
			RenderInterface::RenderShadowMap(pLight->GetShadowSRV(), -zFar, -zNear);
	}
}

//////////////////////////////////////////////////////////////////////////////
bool Light::LoadParameters(NvParameterized::Handle& handle)
{
	// load lights
	NvParameterized::Handle lightsHandle(handle);
	if (handle.getChildHandle(handle.getInterface(), "lights", lightsHandle) == NvParameterized::ERROR_NONE)
	{
		int numLights = 0;
		lightsHandle.getArraySize(numLights);
		if (numLights > g_Lights.size()) 
			numLights = g_Lights.size();

		for (int idx = 0; idx < numLights; ++idx)
		{
			NvParameterized::Handle lightHandle(lightsHandle);
			if (lightsHandle.getChildHandle(idx, lightHandle) == NvParameterized::ERROR_NONE)
			{
				g_Lights[idx].loadParameters(lightHandle);
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////
bool Light::SaveParameters(NvParameterized::Handle& outHandle)
{
	NvParameterized::Handle lightsHandle(outHandle);
	if (outHandle.getChildHandle(outHandle.getInterface(), "lights", lightsHandle) == NvParameterized::ERROR_NONE)
	{
		int numLights = (int)g_Lights.size();

		lightsHandle.resizeArray(numLights);

		for (int idx = 0; idx < numLights; ++idx)
		{
			NvParameterized::Handle lightHandle(outHandle);
			if (lightsHandle.getChildHandle(idx, lightHandle) == NvParameterized::ERROR_NONE)
			{
				g_Lights[idx].saveParameters(lightHandle);
			}
		}
	}
	return true;
}

// END OF STATIC FUNCTIONS

