// Simple Shader that takes a position and can add color

// Defines the expected vertex data per vertex
struct VS_INPUT
{
    float3 position : POSITION;
};

// Output that gets Interpolated in the Rasterizer
struct VS_OUTPUT
{
    float4 position : SV_Position;
};

// Gets applied to every vertex
VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = float4(input.position, 1.0f);
    return output;
}

// Lerp vertex data that is converted to a pixel on the screen
float4 PS(VS_OUTPUT input) : SV_TARGET
{
    return float4(1, 1, 0, 1);
}
