// CriticalCore_Upscale.hlsl
// Fullscreen tinted upscale pass for Critical Core 2.
//
// The whole 2D scene is rendered into a 256x224 offscreen RenderTarget. This
// shader draws a fullscreen screen-quad (MeshBuilder::CreateScreenQuadPX, NDC
// positions in [-1,+1]) that samples a 256x224 source RT and writes it to the
// backbuffer. The letterbox / aspect-correct fit is done CPU-side by setting a
// custom D3D11 viewport before the draw, so the VS is a pure passthrough.
//
// The sampler bound to s0 is a POINT sampler (no linear filtering) to keep the
// pixel-art crisp when upscaled to the window.
//
// A per-draw RGBA `tint` (cbuffer b0) modulates the sample. This reproduces the
// GM oRender Draw_64 composite, where both the blurred base AND the additive
// sharp copy are drawn with make_color_hsv(0,0,255*0.7) (a 70% grey tint).
// Default tint (1,1,1,1) leaves the sample unchanged (plain passthrough upscale).

cbuffer UpscaleBuffer : register(b0)
{
    float4 tint; // per-draw RGBA modulation (0.7 grey for the bloom composite)
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
    return textureMap0.Sample(textureSampler, input.texCoord) * tint;
}
