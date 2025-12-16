// Simple Shader that takes a local position, convertes to NDC space,
// and sets the color.

// Information is passed to the GPU thrrough a buffer
cbuffer TransformBuffer : register(b0)
{
    matrix wvp;
};

// Defines the expected vertex data per vetrex
struct VS_INPUT
{
    float3 position : POSITION;
    float4 color : COLOR;
};

// Output that gets Interpolated in the Rasterizer
struct VS_OUTPUT
{
    float4 position : SV_Position;
    float4 color : COLOR;
};

// Gets applied to every vertex
VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = mul(float4(input.position, 1.0f), wvp);
    output.color = input.color;
    return output;
}

// Lerped vetex data that is converted to a pixel on the screen
float4 PS(VS_OUTPUT input) : SV_TARGET
{
    return input.color;
}