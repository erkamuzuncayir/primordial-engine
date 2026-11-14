/////////////
// GLOBALS //
/////////////
cbuffer PerPassBuffer : register(b0)
{
    column_major matrix viewMatrix;
    column_major matrix projectionMatrix;
    column_major matrix inverseViewMatrix;
    column_major matrix inverseProjectionMatrix;

    float time;
    float deltaTime;

    float2 resolution;
    float2 inverseResolution;

    float4 ambientLightColor;
    float ambientIntensity;
    float3 lightDirection;
    float4 lightColor;
};

cbuffer PerObjectBuffer : register(b1)
{
    column_major matrix worldMatrix;
};

cbuffer PerMaterialBuffer : register(b2)
{
    float4 diffuseColor;
    float3 specularColor;
    float specularPower;
    float2 tiling;
    float2 offset;
}

Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s0);

//////////////
// TYPEDEFS //
//////////////

struct VSInput
{
    float4 position : POSITION;
    float3 normal   : NORMAL;
    float3 tangent  : TEXCOORD0;
    float2 tex      : TEXCOORD1;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal   : NORMAL;
    float3 tangent  : TEXCOORD0;
    float2 tex      : TEXCOORD1;
};

////////////////////////////////////////////////////////////////////////////////
// Vertex Shader
////////////////////////////////////////////////////////////////////////////////
PSInput VSMain(VSInput input)
{
    PSInput output;

    input.position.w = 1.0f;

    output.position = mul(worldMatrix, input.position);
    output.position = mul(viewMatrix, output.position);
    output.position = mul(projectionMatrix, output.position);

    output.tex = input.tex;

    output.normal = mul((float3x3)worldMatrix, input.normal);
    output.normal = normalize(output.normal);

    return output;
}

////////////////////////////////////////////////////////////////////////////////
// Pixel Shader
////////////////////////////////////////////////////////////////////////////////
float4 PSMain(PSInput input) : SV_TARGET
{
    float4 textureColor;
    float3 lightDir;
    float lightIntensity;
    float4 color;

    textureColor = shaderTexture.Sample(SampleType, input.tex);

    lightDir = -lightDirection;
    lightIntensity = saturate(dot(input.normal, lightDir));
    color = saturate(diffuseColor * lightIntensity);
    color = color * textureColor;

    return color;
}