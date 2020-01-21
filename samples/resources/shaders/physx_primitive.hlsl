#include "common_buffers.hlsl"
#include "lighting.hlsl"

struct VS_INPUT
{
	float3 position : POSITION0;
	float3 normal : NORMAL0;
};

struct VS_OUTPUT
{
	float4 position : SV_POSITION;
	float4 worldPos : POSITION0;
	float3 normal : NORMAL0;
};

VS_OUTPUT VS(VS_INPUT iV)
{
	VS_OUTPUT oV;

	float4 worldSpacePos = mul(float4(iV.position, 1.0f), model);
	oV.position = mul(worldSpacePos, viewProjection);

	oV.worldPos = worldSpacePos;

	// normals
	float3 worldNormal = mul(iV.normal,  (float3x3)model);
	oV.normal = worldNormal;

	return oV;
}

float4 PS(VS_OUTPUT iV) : SV_Target0
{
	float3 lightColor = CalcPixelLight(defaultColor.xyz, iV.worldPos.xyz, iV.normal);	
	return float4(lightColor, 1);
}