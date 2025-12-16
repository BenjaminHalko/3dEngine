// Shadow Effect Shader
// Stores Light Data for Objects casting Shadows

cbuffer TransformBuffer : register(b0)
{
    matrix wvp;
};

struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 texCoord : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 position : SV_Position;
    float4 lightNDCPosition : TEXCOORD;
};

VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = mul(float4(input.position, 1.0f), wvp);
    output.lightNDCPosition = output.position; // Scaled based on where it is relative to the world

    return output;
}

float4 PS(VS_OUTPUT input) : SV_Target
{
    // Depth value for shadow mapping
    float depth = saturate(1.0f - (input.lightNDCPosition.z / input.lightNDCPosition.w));
    return float4(depth, 1.0f, 1.0f, 1.0f);
}
