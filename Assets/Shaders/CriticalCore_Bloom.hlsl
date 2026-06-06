// CriticalCore_Bloom.hlsl
// Faithful oRender/Draw_64.gml bloom composite, done in ONE opaque pass.
//
// The WORLD layer (everything that is NOT screen-space UI) is rendered into a
// 256x224 scene buffer, then separably blurred (CriticalCore_Blur.hlsl). This
// pass composites the sharp scene and its blurred copy into a 256x224 buffer:
//
//   output = scene*0.7 + HBlur(VBlur(scene))*0.7   ==   (scene + blur) * tint
//
// The GM original layers a normal-blend blurred surface then an additive sharp
// copy; doing it as a single-pass two-texture add is mathematically identical
// (the scene is opaque, addition commutes) AND avoids a separate additive
// backbuffer blend (which the dxmt/vkd3d backend renders unreliably after an
// opaque draw). The screen-space UI (HUD / score / menu) is drawn SHARP on top
// of this composite AFTERWARDS, so the UI never picks up the blur.
//
//   t0 (sceneTex) - the SHARP world, sampled POINT (s0) to keep pixel-art crisp.
//   t1 (blurTex)  - the blurred world, sampled LINEAR (s1) for a smooth glow.
//   tint (b0)     - 0.7 grey (GM make_color_hsv(0,0,255*0.7)) applied to both.

cbuffer BloomBuffer : register(b0)
{
    float4 tint;
}

Texture2D sceneTex : register(t0);
Texture2D blurTex : register(t1);
SamplerState pointSampler : register(s0);
SamplerState linearSampler : register(s1);

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
    float4 scene = sceneTex.Sample(pointSampler, input.texCoord);
    float4 blur = blurTex.Sample(linearSampler, input.texCoord);
    float4 result = (scene + blur) * tint;
    result.a = 1.0f;
    return result;
}
