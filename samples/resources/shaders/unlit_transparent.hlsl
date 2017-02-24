#include "common_buffers.hlsl"

struct VS_INPUT
{
	float3 position : POSITION0;
	float3 normal : NORMAL0;
};

struct VS_OUTPUT
{
	float4 position : SV_POSITION;
};

VS_OUTPUT VS(VS_INPUT iV)
{
	VS_OUTPUT oV;

	float4 worldSpacePos = mul(float4(iV.position, 1.0f), model);
	oV.position = mul(worldSpacePos, viewProjection);

	return oV;
}

float4 PS(VS_OUTPUT iV) : SV_Target0
{
	return defaultColor;
}