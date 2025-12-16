// Terrain Effect file that blends 2 Textures based on a Blend Map

cbuffer TransformBuffer : register(b0)
{
    matrix world;
    matrix wvp;
    matrix lwvp;
    float3 viewPosition;
}

cbuffer LightBuffer : register(b1)
{
    float4 lightAmbient;
    float4 lightDiffuse;
    float4 lightSpecular;
    float3 lightDirection;
}

cbuffer MaterialBuffer : register(b2)
{
    float4 materialEmissive;
    float4 materialAmbient;
    float4 materialDiffuse;
    float4 materialSpecular;
    float materialShininess;
}

cbuffer SettingBuffer : register(b3)
{
    bool useShadowMap;
    float depthBias;
    float lowHeight;
    float blendHeight;
}

Texture2D lowTextureMap : register(t0);
Texture2D highTextureMap : register(t1);
Texture2D shadowMap : register(t2);

SamplerState textureSampler : register(s0);

struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 texCoord : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 position : SV_Position;
    float3 worldNormal : NORMAL;
    float3 dirToLight : TEXCOORD0;
    float3 dirToView : TEXCOORD1;
    float2 texCoord : TEXCOORD2;
    float4 lightNDCPosition : TEXCOORD3;
    float3 worldPosition : TEXCOORD4;
};

VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;

    output.position = mul(float4(input.position, 1.0f), wvp);
    output.worldNormal = mul(input.normal, (float3x3) world);
    output.worldPosition = mul(float4(input.position, 1.0f), world);
    output.dirToLight = -lightDirection;
    output.dirToView = normalize(viewPosition - output.worldPosition.xyz);
    output.texCoord = input.texCoord;
    if (useShadowMap)
    {
        output.lightNDCPosition = mul(float4(input.position, 1.0f), lwvp);
    }
    return output;
}

float4 PS(VS_OUTPUT input) : SV_Target
{
    float3 n = normalize(input.worldNormal);
    float3 light = normalize(input.dirToLight);
    float3 view = normalize(input.dirToView);

    // Emmissive
    float4 emissive = materialEmissive;

    // Ambient
    float4 ambient = lightAmbient * materialAmbient;

    // Diffuse
    float d = saturate(dot(light, n));
    float4 diffuse = d * lightDiffuse * materialDiffuse;

    // Specular
    float r = reflect(-light, n);
    float base = saturate(dot(r, view));
    float s = pow(base, materialShininess);
    float4 specular = s * lightSpecular * materialSpecular;

    float4 lowMapColor = lowTextureMap.Sample(textureSampler, input.texCoord);
    float4 highMapColor = highTextureMap.Sample(textureSampler, input.texCoord);
    float4 diffuseColor = lowMapColor;

    if (input.worldPosition.y > lowHeight + blendHeight)
    {
        diffuseColor = highMapColor;
    }
    else
    {
        float t = saturate((input.worldPosition.y - lowHeight) / blendHeight);
        diffuseColor = lerp(lowMapColor, highMapColor, t);
    }

    float4 finalColor = (emissive + ambient + diffuse) * diffuseColor + specular;

    if (useShadowMap)
    {
        float actualDepth = 1.0f - (input.lightNDCPosition.z / input.lightNDCPosition.w);
        float2 shadowUV = input.lightNDCPosition.xy / input.lightNDCPosition.w;
        float u = (shadowUV.x + 1.0f) * 0.5f;
        float v = 1.0f - (shadowUV.y + 1.0f) * 0.5f;
        if (saturate(u) == u && saturate(v) == v)
        {
            float4 savedColor = shadowMap.Sample(textureSampler, float2(u, v));
            float savedDepth = savedColor.r;
            if (savedDepth > actualDepth + depthBias)
            {
                finalColor = (emissive + ambient) * diffuseColor;
            }
        }
    }

    return finalColor;
}
