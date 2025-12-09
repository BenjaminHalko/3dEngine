#pragma once

#include <Engine/Inc/Engine.h>

class BalatroEffect
{
public:
    void Initialize(const std::filesystem::path& filePath);
    void Terminate();

    void Begin();
    void End();

    void Update(float deltaTime);
    void Render(const Engine::Graphics::RenderObject& renderObject);

    void DebugUI();

private:
    struct BalatroData
    {
        float time = 0.0f;
        float spinTime = 0.0f;
        float contrast = 3.5f;
        float spinAmount = 0.25f;
        
        Engine::Graphics::Color colour1;
        Engine::Graphics::Color colour2;
        Engine::Graphics::Color colour3;
        
        Engine::Math::Vector2 screenSize;
        float pixelFilter = 740.0f;
        float spinEase = 0.5f;
        
        Engine::Math::Vector2 offset;
        Engine::Math::Vector2 padding;
    };

    using BalatroBuffer = Engine::Graphics::TypedConstantBuffer<BalatroData>;
    BalatroBuffer mBalatroBuffer;

    Engine::Graphics::VertexShader mVertexShader;
    Engine::Graphics::PixelShader mPixelShader;
    Engine::Graphics::Sampler mSampler;

    float mTime = 0.0f;
    float mSpinTime = 0.0f;
    float mContrast = 3.5f;
    float mSpinAmount = 0.5f;
    float mPixelFilter = 700.0f;
    float mSpinEase = 0.5f;
    Engine::Math::Vector2 mOffset = { 0.0f, 0.0f };
    float mTimeSpeed = 1.0f;
    float mSpinSpeed = 1.0f;
    bool mPaused = false;

    // Default Balatro colors
    Engine::Graphics::Color mColour1 = { 0.871f, 0.267f, 0.231f, 1.0f };  // Red
    Engine::Graphics::Color mColour2 = { 0.0f, 0.42f, 0.706f, 1.0f };     // Blue
    Engine::Graphics::Color mColour3 = { 0.086f, 0.137f, 0.145f, 1.0f };  // Dark
};
