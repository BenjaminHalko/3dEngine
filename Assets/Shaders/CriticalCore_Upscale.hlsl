// CriticalCore_Upscale.hlsl
// Fullscreen passthrough upscale pass for Critical Core 2.
//
// The whole 2D scene is rendered into a 256x224 offscreen RenderTarget. This
// shader draws a fullscreen screen-quad (MeshBuilder::CreateScreenQuadPX, NDC
// positions in [-1,+1]) that samples that RT and writes it to the backbuffer.
// The letterbox / aspect-correct fit is done CPU-side by setting a custom
// D3D11 viewport before the draw, so this shader is a pure passthrough.
//
// The sampler bound to s0 is a POINT sampler (no linear filtering) to keep the
// pixel-art crisp when upscaled to the window.

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
    return textureMap0.Sample(textureSampler, input.texCoord);
}
