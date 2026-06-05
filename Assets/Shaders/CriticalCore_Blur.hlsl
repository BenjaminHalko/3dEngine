// CriticalCore_Blur.hlsl
// Faithful HLSL port of Critical Core 2's GameMaker GLSL shader shBlur.fsh
// (5-step directional gaussian, sigma 0.6). Used for the in-game title/core
// glow only (the GM `shBlur` usage) as a SEPARABLE two-pass blur.
//
// ---------------------------------------------------------------------------
// Source (shaders/shBlur/shBlur.fsh):
//   uniform vec2  blur_vector;                       // direction of this pass
//   const  vec2   texel_size  = vec2(1/256, 1/224);  // internal-res texel
//   const  float  blur_steps  = 5.0;                 // taps each side
//   const  float  sigma       = 0.6;                 // gaussian sigma
//   weight(pos) = exp(-(pos*pos) / (2*sigma*sigma));
//   main: accumulate centre + 5 symmetric taps stepped along
//         blur_vector * texel_size, normalised by total_weight.
//
// GLSL -> HLSL mapping:
//   texture2D(tex,uv) -> textureMap0.Sample(textureSampler, uv)
//   vec2/vec4         -> float2/float4
//   gm_BaseTexture    -> textureMap0 (t0)   gl_FragColor -> SV_Target
//   varyings v_vTexcoord -> VS_OUTPUT.texCoord
//   v_vColour (GM draw colour) is DROPPED: VertexPX carries no per-vertex
//   colour and the glow is drawn white, so multiplying by 1 is equivalent.
//
// ---------------------------------------------------------------------------
// cbuffer BlurBuffer (register b0) — 16 bytes, single 16-byte aligned slot:
//   offset  0 : float2 blur_vector   // pass direction in TEXELS, NOT pixels.
//                                     //   pass 1 (horizontal) = (1, 0)
//                                     //   pass 2 (vertical)   = (0, 1)
//   offset  8 : float2 texel_size    // 1/internalRes; default (1/256, 1/224).
//   offset 16 : (end — exactly one float4 register, no tail padding needed)
//
// TWO-PASS (SEPARABLE) USAGE — ping-pong render targets:
//   pass 1: src = glow source, dst = RT_A, blur_vector = (1, 0)  // horizontal
//   pass 2: src = RT_A,        dst = RT_B, blur_vector = (0, 1)  // vertical
//   RT_B then holds the fully blurred glow; composite additively over the
//   title/core. Both passes share this same VS/PS; only blur_vector changes.
//   texel_size stays (1/256, 1/224) for both passes (internal 256x224 space).
// ---------------------------------------------------------------------------

cbuffer BlurBuffer : register(b0)
{
    float2 blur_vector;  // pass direction in texels: (1,0) horizontal / (0,1) vertical
    float2 texel_size;   // 1/internalRes; default (1.0/256.0, 1.0/224.0)
}

Texture2D    textureMap0    : register(t0);
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

// Passthrough VS for the screen / sprite quad (VertexPX: position + uv).
VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = float4(input.position, 1.0f);
    output.texCoord = input.texCoord;
    return output;
}

// Gaussian weight (shBlur.fsh weight(), sigma 0.6).
static const float sigma = 0.6f;

float weight(float pos)
{
    return exp(-(pos * pos) / (2.0f * sigma * sigma));
}

float4 PS(VS_OUTPUT input) : SV_Target
{
    const float blur_steps = 5.0f;

    float4 blurred_col = textureMap0.Sample(textureSampler, input.texCoord);

    float2 sampleUV;
    float  sample_weight;
    float  total_weight = 1.0f;

    [unroll]
    for (float offset = 1.0f; offset <= blur_steps; offset += 1.0f)
    {
        sample_weight = weight(offset / (blur_steps + 1.0f));
        total_weight += 2.0f * sample_weight;

        sampleUV = input.texCoord + offset * texel_size * blur_vector;
        blurred_col += textureMap0.Sample(textureSampler, sampleUV) * sample_weight;

        sampleUV = input.texCoord - offset * texel_size * blur_vector;
        blurred_col += textureMap0.Sample(textureSampler, sampleUV) * sample_weight;
    }

    return blurred_col / total_weight;
}
