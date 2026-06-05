// Critical Core 2 - 2D sprite shader (tinted textured quad)
// Used by Engine::CriticalCore::Render2D. Drives a unit sprite quad through a
// per-draw world-view-projection (y-DOWN orthographic, 256x224 source space)
// and modulates the sampled texel by a per-draw RGBA tint.

cbuffer SpriteBuffer : register(b0)
{
    matrix wvp;   // world * orthoY-down, transposed on the CPU side
    float4 tint;  // per-draw RGBA modulation
}

Texture2D textureMap : register(t0);
SamplerState textureSampler : register(s0);

struct VS_INPUT
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD;
};

VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = mul(float4(input.position, 1.0f), wvp);
    output.texCoord = input.texCoord;
    return output;
}

float4 PS(VS_OUTPUT input) : SV_Target
{
    return textureMap.Sample(textureSampler, input.texCoord) * tint;
}
