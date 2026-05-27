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

cbuffer PerObjectBuffer : register(b1)
{
    column_major matrix worldMatrix;
};

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

PixelInputType VSMain(VertexInputType input)
{
    PixelInputType output;

    float4 pos = float4(input.position, 1.0f);

    output.position = mul(worldMatrix, pos);
    output.position = mul(g_viewMatrix, output.position);
    output.position = mul(g_projMatrix, output.position);
    
    output.tex = input.tex;
      
    output.normal = mul((float3x3)worldMatrix, input.normal);
	
    output.normal = normalize(output.normal);

    return output;
}
