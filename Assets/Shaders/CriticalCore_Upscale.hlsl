// CriticalCore_Upscale.hlsl
// Final point-sampled upscale of the composited 256x224 buffer to the window.
//
// The bloomed world + the sharp screen-space UI have already been composited
// into a 256x224 buffer at native resolution. This pass simply samples that
// buffer with a POINT sampler and writes it to the letterbox rect on the
// backbuffer (the aspect-correct fit is a CPU-side D3D11 viewport, so the VS is
// a pure passthrough). tint is (1,1,1,1) for a plain passthrough.

cbuffer UpscaleBuffer : register(b0)
{
    float4 tint;
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
    output.position = float4(input.position, 1.0);
    output.texCoord = input.texCoord;
    return output;
}

float4 PS(VS_OUTPUT input) : SV_Target
{
    float4 color = textureMap0.Sample(textureSampler, input.texCoord) * tint;
    color.a = 1.0f;
    return color;
}
