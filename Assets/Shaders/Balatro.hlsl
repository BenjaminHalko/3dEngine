// Balatro Background Shader - Converted from LÖVE2D GLSL
// A swirling psychedelic paint effect

cbuffer BalatroBuffer : register(b0)
{
    float time;
    float spinTime;
    float contrast;
    float spinAmount;

    float4 colour1;
    float4 colour2;
    float4 colour3;

    float2 screenSize;
    float pixelFilter;
    float spinEase;

    float2 offset;
    float2 padding;
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
    output.position = float4(input.position, 1.0);
    output.texCoord = input.texCoord;
    return output;
}

float4 PS(VS_OUTPUT input) : SV_Target
{
    float2 screen_coords = input.texCoord * screenSize;

    // Convert to UV coords (0-1) and floor for pixel effect
    float screenLen = length(screenSize);
    float pixel_size = screenLen / pixelFilter;
    float2 uv = (floor(screen_coords * (1.0 / pixel_size)) * pixel_size - 0.5 * screenSize) / screenLen - offset;
    float uv_len = length(uv);

    // Adding in a center swirl, changes with time
    float speed = (spinTime * spinEase * 0.2) + 302.2;
    float new_pixel_angle = atan2(uv.y, uv.x) + speed - spinEase * 20.0 * (1.0 * spinAmount * uv_len + (1.0 - 1.0 * spinAmount));
    float2 mid = (screenSize / screenLen) / 2.0;
    uv = float2(uv_len * cos(new_pixel_angle) + mid.x, uv_len * sin(new_pixel_angle) + mid.y) - mid;

    // Now add the paint effect to the swirled UV
    uv *= 30.0;
    speed = time * 2.0;
    float2 uv2 = float2(uv.x + uv.y, uv.x + uv.y);

    [unroll]
    for (int i = 0; i < 5; i++)
    {
        uv2 += sin(max(uv.x, uv.y)) + uv;
        uv += 0.5 * float2(cos(5.1123314 + 0.353 * uv2.y + speed * 0.131121), sin(uv2.x - 0.113 * speed));
        uv -= 1.0 * cos(uv.x + uv.y) - 1.0 * sin(uv.x * 0.711 - uv.y);
    }

    // Make the paint amount range from 0 - 2
    float contrast_mod = (0.25 * contrast + 0.5 * spinAmount + 1.2);
    float paint_res = min(2.0, max(0.0, length(uv) * 0.035 * contrast_mod));
    float c1p = max(0.0, 1.0 - contrast_mod * abs(1.0 - paint_res));
    float c2p = max(0.0, 1.0 - contrast_mod * abs(paint_res));
    float c3p = 1.0 - min(1.0, c1p + c2p);

    float4 ret_col = (0.3 / contrast) * colour1 + (1.0 - 0.3 / contrast) * (colour1 * c1p + colour2 * c2p + float4(c3p * colour3.rgb, c3p * colour1.a));

    return ret_col;
}
