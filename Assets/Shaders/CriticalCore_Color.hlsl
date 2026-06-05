// Critical Core 2 - 2D color shader (flat-shaded VertexPC primitives)
// Used by Engine::CriticalCore::Render2D's color path (filled circle, circle
// outline, thick line). Drives manually-built VertexPC geometry, already laid
// out in 256x224 source-pixel space, through the same per-draw y-DOWN
// orthographic projection as CriticalCore_Sprite.hlsl. The world matrix is the
// identity (geometry is authored directly in pixel space), so the cbuffer holds
// only the transposed ortho. The vertex color carries the per-primitive RGBA;
// there is no texture or sampler on this path.

cbuffer ColorBuffer : register(b0)
{
    matrix wvp;  // (identity world) * orthoY-down, transposed on the CPU side
}

struct VS_INPUT
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct VS_OUTPUT
{
    float4 position : SV_Position;
    float4 color : COLOR;
};

VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = mul(float4(input.position, 1.0f), wvp);
    output.color = input.color;
    return output;
}

float4 PS(VS_OUTPUT input) : SV_Target
{
    return input.color;
}
