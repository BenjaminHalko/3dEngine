// CriticalCore_BrightExtract.hlsl
// Bloom pre-pass: keeps only the bright pixels of the scene RT (core / title /
// walls / player / bubbles) and zeroes the dark navy backdrop, so the separable
// blur that follows produces a selective GLOW rather than a flat full-screen
// haze. Output feeds CriticalCore_Blur.hlsl (H then V), then composites
// additively over the upscaled scene.

cbuffer BrightBuffer : register(b0)
{
    float threshold; // luma below this contributes nothing to the glow
    float knee;      // soft ramp width above the threshold
    float intensity; // glow gain
    float _pad0;
}

Texture2D textureMap0 : register(t0);
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
    output.position = float4(input.position, 1.0f);
    output.texCoord = input.texCoord;
    return output;
}

float4 PS(VS_OUTPUT input) : SV_Target
{
    float3 col = textureMap0.Sample(textureSampler, input.texCoord).rgb;

    // Rec. 601 luma; smoothstep keeps a soft edge so the glow has no hard cutoff.
    float luma = dot(col, float3(0.299f, 0.587f, 0.114f));
    float w = smoothstep(threshold, threshold + knee, luma);

    return float4(col * w * intensity, 1.0f);
}
