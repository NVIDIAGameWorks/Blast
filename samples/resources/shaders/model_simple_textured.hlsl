#include "common_buffers.hlsl"
#include "lighting.hlsl"

SamplerState defaultSampler : register(s0);
Texture2D diffuseTexture : register(t0);

struct VS_INPUT
{
	float3 position : POSITION0;
	float3 normal : NORMAL0;
	float2 uv : TEXCOORD0;
	float health : TEXCOORD1;
};

struct VS_OUTPUT
{
	float4 position : SV_POSITION;
	float4 worldPos : POSITION0;
	float2 uv : TEXCOORD0;
	float3 normal : NORMAL0;
	float health : TEXCOORD1;
};

VS_OUTPUT VS(VS_INPUT iV)
{
	VS_OUTPUT oV;

	float4 worldSpacePos = mul(float4(iV.position, 1.0f), model);
	oV.position = mul(worldSpacePos, viewProjection);

	oV.worldPos = worldSpacePos;

	// normals
	float3 worldNormal = mul(float4(iV.normal, 0.0f), model);
	oV.normal = worldNormal;

	oV.uv = iV.uv;
	oV.health = iV.health;

	return oV;
}

float noise2(float2 co)
{
    return frac(sin(dot(co.xy, float2(12.9898,78.233))) * 43758.5453);
}

float voronoi( float2 x )
{
    int2 p = floor( x );
    float2  f = frac( x );
 
    float res = 8.0;
    for( int j=-1; j<=1; j++ )
    for( int i=-1; i<=1; i++ )
    {
        int2 b = int2( i, j );
        float2  r = float2( b ) - f + noise2( p + b );
        float d = dot( r, r );
 
        res = min( res, d );
    }
    return sqrt( res );
}

float4 PS(VS_OUTPUT iV) : SV_Target0
{
	float4 textureColor = diffuseTexture.Sample(defaultSampler, iV.uv);

	// health cracks hack
	float crack = 1.0f - voronoi(iV.uv * 50.0f);
	crack = smoothstep(0.0f, 0.5f, crack);
	textureColor = textureColor * lerp(1.0f, crack, (1.0f - iV.health) * 0.7f);

	return float4(CalcPixelLight(textureColor, iV.worldPos, iV.normal), 1);
}