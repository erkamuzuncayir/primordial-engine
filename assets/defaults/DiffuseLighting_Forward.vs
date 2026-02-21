////////////////////////////////////////////////////////////////////////////////
// Filename: light.vs
////////////////////////////////////////////////////////////////////////////////

/////////////
// GLOBALS //
/////////////

// SLOT 0: Global Data (Matches your new C++ Struct)
// UpdateFrequency: Once per Frame
cbuffer GlobalUniforms : register(b0)
{
    column_major matrix g_viewMatrix;
    column_major matrix g_projMatrix;
    float3 g_cameraPos;
    float  g_time;
    float3 g_lightDir;
    float  g_ambientIntensity;
    float4 g_lightColor;
};

// SLOT 1: Object Data
// UpdateFrequency: Once per Object
cbuffer PerObjectBuffer : register(b1)
{
    column_major matrix worldMatrix;
};

//////////////
// TYPEDEFS //
//////////////
struct VertexInputType
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float3 tangent  : TEXCOORD0;
    float2 tex      : TEXCOORD1;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float3 normal   : NORMAL;
    float3 tangent  : TEXCOORD0;
    float2 tex      : TEXCOORD1;
};

////////////////////////////////////////////////////////////////////////////////
// Vertex Shader
////////////////////////////////////////////////////////////////////////////////
PixelInputType VSMain(VertexInputType input)
{
    PixelInputType output;

	// Change the position vector to be 4 units for proper matrix calculations.
    float4 pos = float4(input.position, 1.0f);

	// Calculate the position of the vertex against the world, view, and projection matrices.
	output.position = mul(worldMatrix, pos);      // Önce World
    output.position = mul(g_viewMatrix, output.position); // Sonra View
    output.position = mul(g_projMatrix, output.position); // Sonra proj
    
	// Store the texture coordinates for the pixel shader.
	output.tex = input.tex;
    
	// Calculate the normal vector against the world matrix only.
    output.normal = mul((float3x3)worldMatrix, input.normal);
	
    // Normalize the normal vector.
    output.normal = normalize(output.normal);

    return output;
}