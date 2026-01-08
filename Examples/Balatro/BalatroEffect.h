#pragma once

#include <Engine/Inc/Engine.h>

struct BalatroPreset
{
    std::string name;
    Engine::Graphics::Color colour1;
    Engine::Graphics::Color colour2;
    Engine::Graphics::Color colour3;
    float contrast;
    float spinAmount;
};

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

    // Preset system
    void SetPreset(int index, float transitionTime = 1.0f);
    void SetPreset(const std::string& name, float transitionTime = 1.0f);
    int GetCurrentPresetIndex() const { return mCurrentPresetIndex; }
    const std::vector<BalatroPreset>& GetPresets() const { return mPresets; }

private:
    // Helper to convert hex color to Color
    static Engine::Graphics::Color HexToColor(uint32_t hex);

    // Lerp between colors
    static Engine::Graphics::Color LerpColor(const Engine::Graphics::Color& a, const Engine::Graphics::Color& b, float t);

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

    // Current colors (interpolated during transitions)
    Engine::Graphics::Color mColour1 = { 0.871f, 0.267f, 0.231f, 1.0f };
    Engine::Graphics::Color mColour2 = { 0.0f, 0.42f, 0.706f, 1.0f };
    Engine::Graphics::Color mColour3 = { 0.086f, 0.137f, 0.145f, 1.0f };

    // Preset system
    std::vector<BalatroPreset> mPresets;
    int mCurrentPresetIndex = 0;

    // Transition state
    bool mIsTransitioning = false;
    float mTransitionTime = 1.0f;
    float mTransitionProgress = 0.0f;
    Engine::Graphics::Color mStartColour1;
    Engine::Graphics::Color mStartColour2;
    Engine::Graphics::Color mStartColour3;
    float mStartContrast = 3.5f;
    float mStartSpinAmount = 0.5f;
    Engine::Graphics::Color mTargetColour1;
    Engine::Graphics::Color mTargetColour2;
    Engine::Graphics::Color mTargetColour3;
    float mTargetContrast = 3.5f;
    float mTargetSpinAmount = 0.5f;

    void InitializePresets();
};
