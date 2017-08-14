struct VS_INPUT
{
	float3 position : POSITION0;
	float3 color : COLOR0;
};

struct VS_OUTPUT
{
	float4 position : SV_POSITION;
	float3 color : COLOR0;
};

VS_OUTPUT VS(VS_INPUT iV)
{
	VS_OUTPUT oV;

	oV.position = float4(iV.position.x * 2.0 - 1.0, 1.0 - iV.position.y * 2.0, 0.0, 1.0);
	oV.color = iV.color;

	return oV;
}

float4 PS(VS_OUTPUT iV) : SV_Target0
{
	return float4(iV.color, 1.0);
}