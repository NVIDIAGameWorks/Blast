#ifndef COMMON_BUFFERS_HLSL
#define COMMON_BUFFERS_HLSL

cbuffer Camera : register(b0)
{
	row_major matrix viewProjection;
	row_major matrix projectionInv;
	float3 viewPos;
};

struct Light 
{
	int				m_enable;
	float3			m_dir;

	int				m_useShadows;
	float3			m_color;

	float3			m_ambientColor;
	int				m_isEnvLight;

	int				m_lhs;
	int				_reserved1;
	int				_reserved2;
	int				_reserved3;

	float			m_depthBias;
	float			m_depthGain;
	int				m_useEnvMap;
	float			m_intensity;

	row_major float4x4	m_viewMatrix;
	row_major float4x4	m_lightMatrix;
};

cbuffer World : register(b1)
{
	float3 ambientColor;
	float3 pointLightPos;
	float3 pointLightColor;
	float3 dirLightDir;
	float specularPower;
	float3 dirLightColor;
	float specularIntensity;
	float g_flatNormal;
	float g_wireFrameOver;
	float g_useLighting;
	Light g_Light[4];
};

cbuffer Object : register(b2)
{
	row_major matrix worldMatrix;
	float4 m_diffuseColor;
	float4 m_specularColor;
	float m_useDiffuseTexture;
	float m_useSpecularTexture;
	float m_useNormalTexture;
	float m_specularShininess;
	float selected;
};

#endif