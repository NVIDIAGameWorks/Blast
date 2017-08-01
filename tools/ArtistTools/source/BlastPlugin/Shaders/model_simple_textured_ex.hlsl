#include "common_buffers_ex.hlsl"
#include "lighting_ex.hlsl"

SamplerState defaultSampler : register(s0);
Texture2D diffuseTexture : register(t0);
Texture2D specularTexture : register(t1);
Texture2D normalTexture : register(t2);
Texture2D envTexture : register(t3);

struct VS_INPUT
{
	float3 position : POSITION0;
	float3 normal : NORMAL0;
	float3 tangent : TANGENT0;
	float2 uv : TEXCOORD0;
	float health : TEXCOORD1;
};

struct VS_OUTPUT
{
	float4 position : SV_POSITION;
	float4 worldPos : POSITION0;
	float3 normal : NORMAL0;
	float3 tangent : TANGENT0;
	float2 uv : TEXCOORD0;
	float health : TEXCOORD1;
};

VS_OUTPUT VS(VS_INPUT iV)
{
	VS_OUTPUT oV;

	float4 worldSpacePos = mul(float4(iV.position, 1.0f), worldMatrix);
	oV.position = mul(worldSpacePos, viewProjection);

	oV.worldPos = worldSpacePos;

	// normals
	float3 worldNormal = mul(float4(iV.normal, 0.0f), worldMatrix);
	oV.normal = worldNormal;
	
	oV.tangent = normalize(iV.tangent);

	oV.uv = iV.uv;
	oV.health = iV.health;

	return oV;
}

inline float2 GetLightTexCoord(float3 lightdirection)
{
	const float M_PI = 3.1415923;
	float u = 0.5f + 0.5f * atan2(lightdirection.y, lightdirection.x) / M_PI;
	float v = 0.5f - 0.5f * lightdirection.z;
	return float2(u,v);
}

inline float3 computeDiffuseLighting( 
	float3 diffuse, float3 lightdirection, float3 surfacenormal )
{
	float ratio = max(0, dot(surfacenormal, lightdirection));
	return diffuse * ratio;
}

inline float3 computeSpecularLighting( 
	float3 lightcolor, float3 lightdirection, float3 surfacenormal, 
	float3 viewvector, float3 specularity, float shininess)
{
	float3 H = normalize(viewvector + surfacenormal);
	float NdotH = max(0, dot(H, surfacenormal));
	float specular	= pow(NdotH, shininess);
	float3 output = specular * lightcolor * specularity;
	return output;
}

float4 PS(VS_OUTPUT iV) : SV_Target0
{
	float3 diffuseColor = m_diffuseColor.xyz;
	if(m_useDiffuseTexture > 0)
	{
		diffuseColor = diffuseTexture.Sample(defaultSampler, iV.uv).xyz;
	}
	float3 specularColor = m_specularColor.xyz;
	if(m_useSpecularTexture > 0)
	{
		specularColor = specularTexture.Sample(defaultSampler, iV.uv).xyz;
	}	
	float3 N = normalize(iV.normal.xyz);
	if (m_useNormalTexture > 0)
	{
		float3 normalColor = normalTexture.Sample(defaultSampler, iV.uv).xyz;
		normalColor = (normalColor - 0.5) * 2.0f;
		float3 T = normalize(iV.tangent.xyz);
		float3 B = normalize(cross(T, N));
		float3 PN = N;	
		PN += normalColor.x * T;
		PN += normalColor.y * B;
		PN += normalColor.z * N;
		N = normalize(PN);
	}
	
	float4 color = float4(0,0,0,1);
	
	float3 P = iV.worldPos.xyz;
	float3 E = normalize(viewPos.xyz - P);
	float shininess = m_specularShininess;

	float3 albedo = diffuseColor.rgb;
	float3 specularity = specularColor.rgb;

	float3 diffuse = 0;
	float3 specular = 0;
	float3 ambient = 0;

	[unroll]
	for (int i = 0; i < 4; i++)
	{
		Light L = g_Light[i];

		if (L.m_enable)
		{
			float3 Ldiffuse = 0;
			float3 Lspecular = 0;
			float3 Ldir = 0;

			if (L.m_isEnvLight)
			{
				Ldir = N;
				float2 texcoords = GetLightTexCoord(Ldir);
				float3 Lcolor = (L.m_useEnvMap) ? envTexture.Sample(defaultSampler,texcoords.xy).rgb : L.m_color;
				Lcolor *= L.m_intensity;
				Ldiffuse = Lspecular = Lcolor;
			}
			else
			{
				Ldir = L.m_dir;
				Ldiffuse = Lspecular = L.m_intensity * L.m_color;
			}

			diffuse += computeDiffuseLighting( Ldiffuse, Ldir, N);
			specular += computeSpecularLighting( Lspecular, Ldir, N, E, specularity, shininess);
			ambient += L.m_ambientColor;
		}
	}

	color.rgb = (ambient + diffuse) * albedo + specular;

	if(selected > 0)
	{
		if(color.r > 0.5)
		{
			color.r = 0.5;
		}
		else
		{
			color.r += 0.5;
		}
	}	
	return color;
}