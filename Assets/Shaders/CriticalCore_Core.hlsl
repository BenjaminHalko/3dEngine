// Critical Core 2 - The Core's volumetric visual.
// Faithful HLSL port of GameMaker GLSL shCore.fsh + shCore.vsh (raymarched
// fractal-noise nebula). Compiled at runtime via D3DCompile (entry points VS/PS,
// targets vs_5_0/ps_5_0 - same path as Balatro / CriticalCore_Sprite / _Upscale).
//
// ===========================================================================
//  CoreBuffer constant-buffer layout (register b0) - TASK 21 BINDS THIS
// ===========================================================================
//  HLSL cbuffer packing is 16-byte (float4) aligned. A float3 may not straddle
//  a 16-byte boundary; the layout below is hand-verified:
//
//    offset  size  field         notes
//    ------  ----  ------------  -------------------------------------------
//      0      4    iTime         seconds; bound from coreEffectTime / pulse clock
//      4     12    iResolution   (x=256, y=224, z=0) internal render res; .z unused
//     16      4    intensity     core pulse 0..1 (drives the final colour mix)
//     20     12    _pad0         padding -> next 16-byte boundary
//    ------------  ------------
//    total = 32 bytes (2 x float4 rows). 16-byte aligned.
//
//  CPU struct to mirror (std140-equivalent for D3D11):
//    struct CoreData { float iTime; float iResX, iResY, iResZ;     // row 0
//                      float intensity; float pad0, pad1, pad2; }; // row 1
//  i.e. iResolution packs immediately after iTime in row 0; intensity opens row 1.
// ===========================================================================
cbuffer CoreBuffer : register(b0)
{
    float  iTime;        // offset  0
    float3 iResolution;  // offset  4  (256, 224, 0)
    float  intensity;    // offset 16
    float3 _pad0;        // offset 20 -> 32
    float4 tint;         // offset 32 -> 48 (GM draw_set_color on the shader polygon)
}

// VS input = VertexPX (POSITION float3 + TEXCOORD float2), matching the engine's
// sprite/screen-quad vertex format (see CriticalCore_Sprite.hlsl). The Core is
// drawn as a screen-space effect quad: the VS is a straight passthrough (NDC
// position, uv 0..1) mirroring Balatro.hlsl's VS - NO wvp transform, because
// CoreBuffer carries no matrix and the b0 slot is owned by CoreBuffer.
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
    output.position = float4(input.position, 1.0f); // passthrough (already NDC)
    output.texCoord = input.texCoord;               // -> v_vTexcoord in the PS
    return output;
}

// ---------------------------------------------------------------------------
//  Hash-based 3D value noise  (GLSL: fract->frac, mix->lerp, vec*->float*)
// ---------------------------------------------------------------------------
float hash(float n)
{
    return frac(sin(n) * 43758.5453);
}

float noise(float3 x)
{
    float3 p = floor(x);
    float3 f = frac(x);

    f = f * f * (3.0 - 2.0 * f);
    float n = p.x + p.y * 57.0 + 113.0 * p.z;
    return lerp(
        lerp(
            lerp(hash(n + 0.0), hash(n + 1.0), f.x),
            lerp(hash(n + 57.0), hash(n + 58.0), f.x),
            f.y),
        lerp(
            lerp(hash(n + 113.0), hash(n + 114.0), f.x),
            lerp(hash(n + 170.0), hash(n + 171.0), f.x),
            f.y),
        f.z);
}

float3 noise3(float3 x)
{
    return float3(
        noise(x + float3(123.456, 0.567, 0.37)),
        noise(x + float3(0.11, 47.43, 19.17)),
        noise(x));
}

float bias(float x, float b)
{
    return x / ((1.0 / b - 2.0) * (1.0 - x) + 1.0);
}

float gain(float x, float g)
{
    float t = (1.0 / g - 2.0) * (1.0 - (2.0 * x));
    return x < 0.5 ? (x / (t + 1.0)) : (t - x) / (t - 1.0);
}

// ---------------------------------------------------------------------------
//  rotation()  -  axis/angle 3x3 rotation matrix.
//
//  GLSL GOTCHA (matrix major-ness): GLSL `mat3(c0, c1, c2)` builds the matrix
//  COLUMN-major (each vec3 arg is a COLUMN), and `mat3 * vec3` = c0*v.x +
//  c1*v.y + c2*v.z. HLSL `float3x3(a0, a1, a2)` builds ROW-major (each arg is a
//  ROW). To reproduce GLSL's column-major M*v exactly: keep the SAME three
//  vectors (as HLSL rows) and call `mul(v, M)` instead of `mul(M, v)`:
//      mul(v, float3x3(a0,a1,a2)).k = v.x*a0[k] + v.y*a1[k] + v.z*a2[k]
//                                   = (a0*v.x + a1*v.y + a2*v.z)[k]   <-- identical.
//  Hence: build float3x3 with GLSL's column args as rows, then mul(vec, M).
//  (See the map() call site: `colorVec = mul(colorVec, rotation(...))`.)
// ---------------------------------------------------------------------------
float3x3 rotation(float angle, float3 axis)
{
    float s = sin(-angle);
    float c = cos(-angle);
    float oc = 1.0 - c;
    float3 sa = axis * s;
    float3 oca = axis * oc;
    return float3x3(
        oca.x * axis + float3(c, -sa.z, sa.y),
        oca.y * axis + float3(sa.z, c, -sa.x),
        oca.z * axis + float3(-sa.y, sa.x, c));
}

float3 fbm(float3 x, float H, float L, int oc)
{
    float3 v = float3(0.0, 0.0, 0.0);
    float f = 1.0;
    for (int i = 0; i < 10; i++)
    {
        if (i >= oc) break;
        float w = pow(f, -H);
        v += noise3(x) * w;
        x *= L;
        f *= L;
    }
    return v;
}

float3 smf(float3 x, float H, float L, int oc, float off)
{
    float3 v = float3(1.0, 1.0, 1.0);
    float f = 1.0;
    for (int i = 0; i < 10; i++)
    {
        if (i >= oc) break;
        v *= off + f * (noise3(x) * 2.0 - 1.0);
        f *= H;
        x *= L;
    }
    return v;
}

float4 map(float3 p)
{
    p -= float3(1.0, 0.1, 0.0) * iTime * 0.01;
    p *= 4.0;

    float3 axis = 4.0 * fbm(p, 0.5, 2.0, 8);
    float3 colorVec = 0.5 * 5.0 * fbm(p * 0.3, 0.5, 2.0, 7);
    p += colorVec;

    float mag = 0.75e5;
    float3 colorMod = mag * smf(p, 0.7, 2.0, 8, 0.2);
    colorVec += colorMod;

    // GLSL `rotation(...) * colorVec` (column-major) -> HLSL `mul(colorVec, M)`.
    colorVec = mul(colorVec, rotation(3.0 * length(axis), normalize(axis)));
    colorVec *= 0.1;

    float4 res;
    res.xyz = colorVec;
    res.w = length(colorVec) * 8.0;
    res = clamp(res, float4(0.0, 0.0, 0.0, 0.0), float4(1.0, 1.0, 1.0, 1.0));

    return res;
}

float4 raymarch(float3 ro, float3 rd)
{
    float4 sum = float4(0.0, 0.0, 0.0, 0.0);
    float t = 0.1;

    for (int i = 0; i < 64; i++)
    {
        if (sum.a > 0.99) continue;

        float3 pos = ro + t * rd;
        float4 col = map(pos);

        col.a *= 0.35 * (t * 8.0);
        col.rgb *= col.a;

        sum = sum + col * (1.0 - sum.a);
        t += max(0.1, 0.025 * t);
    }

    sum.xyz /= (0.001 + sum.w);
    return clamp(sum, 0.0, 1.0);
}

float4 PS(VS_OUTPUT input) : SV_Target
{
    float2 q = input.texCoord; // v_vTexcoord
    float2 p = -1.0 + 2.0 * q;
    p.x *= iResolution.x / iResolution.y;

    float mo_x = sin(iTime * 0.0125);

    // Camera setup
    float3 ro = 4.0 * normalize(float3(cos(2.75 - 3.0 * mo_x), 0.7 + 1.0, sin(2.75 - 3.0 * mo_x)));
    float3 ta = float3(0.0, 1.0, 0.0);
    float3 ww = normalize(ta - ro);
    float3 uu = normalize(cross(float3(0.0, 1.0, 0.0), ww));
    float3 vv = normalize(cross(ww, uu));
    float3 rd = normalize(p.x * uu + p.y * vv + 1.5 * ww);

    float4 res = raymarch(ro, rd);
    float3 col = res.xyz;

    float intense = dot(col, float3(0.5, 0.5, 0.5));
    // GLSL mix(a,b,t) == HLSL lerp(a,b,t)
    col = lerp(col * (1.0 + intensity), float3(intense, intense, intense), intensity * -0.5);

    // GLSL `gl_FragColor = v_vColour * vec4(col,1.0)`: the nebula body draws white
    // (tint = 1), the HP octagon draws merge_color(#FF005E, red, pulse) so the HP
    // disc is the nebula MODULATED by pink, not a flat fill.
    return float4(col, 1.0) * tint;
}
