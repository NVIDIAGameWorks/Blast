////////////////////////////////////////////////////////////////////////////////
// Filename: color.vs
////////////////////////////////////////////////////////////////////////////////


/////////////
// GLOBALS //
/////////////
cbuffer MatrixBuffer
{
	row_major	matrix worldMatrix;
	row_major	matrix viewMatrix;
	row_major	matrix projectionMatrix;
	float4		g_color;

	int		g_useVertexColor;
	int		g_dummy2;
	int		g_dummy3;
	int		g_dummy4;
};


//////////////
// TYPEDEFS //
//////////////
struct VertexInputType
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};


////////////////////////////////////////////////////////////////////////////////
// Vertex Shader
////////////////////////////////////////////////////////////////////////////////
PixelInputType vs_main(VertexInputType input)
{
    PixelInputType output;
    
	// Calculate the position of the vertex against the world, view, and projection matrices.
    output.position = mul(float4(input.position,1.0), worldMatrix);
    output.position = mul(float4(output.position.xyz,1.0f), viewMatrix);
    output.position = mul(float4(output.position.xyz,1.0f), projectionMatrix);
    
	// Store the input color for the pixel shader to use.
    output.color = input.color;

	if (!g_useVertexColor)
		output.color = g_color;
    
    return output;
}


////////////////////////////////////////////////////////////////////////////////
// Pixel Shader
////////////////////////////////////////////////////////////////////////////////
float4 ps_main(PixelInputType input) : SV_TARGET
{
    return input.color;
}
