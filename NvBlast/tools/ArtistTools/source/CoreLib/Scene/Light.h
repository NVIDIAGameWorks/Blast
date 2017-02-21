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

#include "MathUtil.h"
#ifndef NV_ARTISTTOOLS
#include "ProjectParams.h"
#else
#include "NvParametersTypes.h"
#ifndef NV_PARAMETERIZED_ONLY_LAYOUTS
#include "NvParameterized.h"
#include "NvParameters.h"
#include "NvParameterizedTraits.h"
#include "NvTraitsInternal.h"
#endif
#endif // NV_ARTISTTOOLS
#include "Camera.h"

#include <string>
#include <vector>

#include "LightShaderParam.h"

class GPUShaderResource;
class ShadowMap;

/////////////////////////////////////////////////////////////////////////
// Utility class for light object
/////////////////////////////////////////////////////////////////////////
class CORELIB_EXPORT Light
{
public:
	bool			m_enable;
	bool			m_useShadows;
	bool			m_visualize;
	bool			m_isEnvLight;
	bool			m_useEnvMap;

	bool			m_selected;

	float			m_intensity;
	atcore_float3	m_color;

	std::string		m_name;
	int				m_shadowMapResolutionIndex;
	int				m_lightMapResolution;

	// public API to access 4 lights
	static std::vector<Light>& GetDefaultLights();
	static Light* GetLight(int index);
	static Light* GetFirstSelectedLight();

	static void Initialize();
	static void Shutdown();
	static void FitLightCameras(atcore_float3& center, atcore_float3& extent);
	static void ResetUpDir(bool zup);
	static void ResetLhs(bool lhs);
	static void DrawLights(Camera* pCamera);
	static void FillLightShaderParam(LightShaderParam &param);
	static bool LoadParameters(NvParameterized::Handle& handle);
	static bool SaveParameters(NvParameterized::Handle& outHandle);
	static void RenderShadowMap();

	static bool SetEnvTextureFromFilePath(const char* textureFilePath);
	static const std::string & GetEnvTextureFilePath();
	static GPUShaderResource* GetEnvTextureSRV();
	static GPUShaderResource* GetShadowSRV(int);

	static bool GetLinkLightOption();
	static void SetLinkLightOption(bool val);

public:
	Light();
	~Light();

	enum
	{
		KEY_LIGHT,
		FILL_LIGHT,
		RIM_LIGHT,
		ENV_LIGHT,
	};

	void Orbit(const atcore_float2& delta);
	void Pan(const atcore_float2& delta, const atcore_float3& axisX, const atcore_float3& axisY);

	bool UseLHS() const;

	float GetDistance() const;
	bool SetDistance(float newdistsance);

	void SetShadowMapResolution(int option);
	void BeginShadowMapRendering();
	void EndShadowMapRendering();

	GPUShaderResource* GetShadowSRV();

	atcore_float4x4 GetViewMatrix() const;
	atcore_float4x4 GetProjectionMatrix() const;
	atcore_float4x4 GetLightMatrix() const;

protected:
	void Init();
	void Release();

	bool loadParameters(NvParameterized::Handle& handle);
	bool saveParameters(NvParameterized::Handle& outHandle);

	void SetBounds(atcore_float3* center, atcore_float3* extents = 0);
	void FitBounds(bool updateCenter = false);

	atcore_float3 getLightDirection() const;
	void InitCamera(const atcore_float3& from, const atcore_float3& lookAt);

	void draw(Camera* pCamera);
	void resetUpDir(bool zup);
	void resetLhs(bool zup);

public:

	Camera			m_lightCamera;
	ShadowMap*		m_pShadowMap;

	atcore_float3	m_bbCenter;
	atcore_float3	m_bbExtent;
};
