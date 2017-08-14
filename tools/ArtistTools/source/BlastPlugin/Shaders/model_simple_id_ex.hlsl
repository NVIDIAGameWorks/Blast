#include "common_buffers_ex.hlsl"

struct VS_INPUT
{
	float3 position : POSITION0;
	float3 normal : VERTEX_NORMAL;
	float3 faceNormal : FACE_NORMAL;
	float3 tangent : TANGENT;
	float2 uv : TEXCOORD0;
	float health : TEXCOORD1;
};

struct VS_OUTPUT
{
	float4 position : SV_POSITION;
};

struct PSOut
{
	float vertexid: SV_Target0;
};

VS_OUTPUT VS(VS_INPUT iV)
{
	VS_OUTPUT oV;
	float4 worldSpacePos = mul(float4(iV.position, 1.0f), worldMatrix);
	oV.position = mul(worldSpacePos, viewProjection);
	return oV;
}

PSOut PS(VS_OUTPUT oV) : SV_Target0
{
	PSOut output;
	output.vertexid = selected;
	return output;
}