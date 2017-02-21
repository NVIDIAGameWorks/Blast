#include "common_buffers.hlsl"
#include "lighting.hlsl"

struct VS_INPUT
{
	float3 position : POSITION0;
	float3 normal : NORMAL0;
	float2 uv : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 position : SV_POSITION;
	float4 worldPos : POSITION0;
	float2 uv : TEXCOORD0;
	float3 normal : NORMAL0;
};

float filterwidth(float2 v)
{
  float2 fw = max(abs(ddx(v)), abs(ddy(v)));
  return max(fw.x, fw.y);
}

float2 bump(float2 x) 
{
	return (floor(x/2) + 2.f * max((x/2) - floor(x/2) - .5f, 0.f)); 
}

float checker(float2 uv)
{
  float width = filterwidth(uv);
  float2 p0 = uv - 0.5 * width;
  float2 p1 = uv + 0.5 * width;
  
  float2 i = (bump(p1) - bump(p0)) / width;
  return i.x * i.y + (1 - i.x) * (1 - i.y);
}

VS_OUTPUT VS(VS_INPUT iV)
{
	VS_OUTPUT oV;

	float4 worldSpacePos = mul(float4(iV.position, 1.0f), model);
	oV.position = mul(worldSpacePos, viewProjection);

	oV.uv = iV.uv;

	oV.worldPos = worldSpacePos;

	// normals
	float3 worldNormal = mul(iV.normal,  (float3x3)model);
	oV.normal = worldNormal;

	return oV;
}

float4 PS(VS_OUTPUT iV) : SV_Target0
{
	float4 color = defaultColor;
	color *= 1.0 - 0.25 * checker(iV.uv);
	float3 lightColor = CalcPixelLight(color.xyz, iV.worldPos.xyz, iV.normal);	
	return float4(lightColor, 1);
}