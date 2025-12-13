#include "BalatroEffect.h"

using namespace Engine;
using namespace Engine::Graphics;

namespace
{
    // Attempt smoothstep for smoother easing
    float Smoothstep(float t)
    {
        return t * t * (3.0f - 2.0f * t);
    }
}

Color BalatroEffect::HexToColor(uint32_t hex)
{
    float r = ((hex >> 16) & 0xFF) / 255.0f;
    float g = ((hex >> 8) & 0xFF) / 255.0f;
    float b = (hex & 0xFF) / 255.0f;
    return { r, g, b, 1.0f };
}

Color BalatroEffect::LerpColor(const Color& a, const Color& b, float t)
{
    return {
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t
    };
}

void BalatroEffect::InitializePresets()
{
    mPresets.clear();

    // Colors from original Balatro source (globals.lua and game.lua)
    // Format: HexToColor(0xRRGGBB)

    // === GAME STATES ===

    // Default/Menu - Red and Blue (classic Balatro look)
    mPresets.push_back({
        "Default",
        HexToColor(0xFE5F55),  // RED
        HexToColor(0x009dff),  // BLUE
        HexToColor(0x374244),  // BLACK (dark background)
        3.5f,
        0.5f
    });

    // Small/Big Blind - Green tones
    mPresets.push_back({
        "Small Blind",
        HexToColor(0x50846e),  // BLIND.Small - main green
        HexToColor(0x68ad8e),  // Lighter green (1.3x brightness)
        HexToColor(0x385d4d),  // Darker green (0.7x brightness)
        1.0f,
        0.0f
    });

    // === BOOSTER PACKS ===

    // Tarot Pack - Purple mystical
    mPresets.push_back({
        "Tarot Pack",
        HexToColor(0x8867a5),  // PURPLE
        HexToColor(0xb087d8),  // Lighter purple
        HexToColor(0x272a2c),  // Dark (darken BLACK 0.2)
        1.5f,
        0.0f
    });

    // Spectral Pack - Blue ethereal
    mPresets.push_back({
        "Spectral Pack",
        HexToColor(0x4584fa),  // SECONDARY_SET.Spectral
        HexToColor(0x6aa8ff),  // Lighter blue
        HexToColor(0x272a2c),  // Dark
        2.0f,
        0.0f
    });

    // Planet Pack - Dark cosmic
    mPresets.push_back({
        "Planet Pack",
        HexToColor(0x374244),  // BLACK
        HexToColor(0x485658),  // Slightly lighter
        HexToColor(0x272a2c),  // Darker
        3.0f,
        0.0f
    });

    // Buffoon Pack - Orange/filter
    mPresets.push_back({
        "Buffoon Pack",
        HexToColor(0xff9a00),  // FILTER (orange)
        HexToColor(0xffb84d),  // Lighter orange
        HexToColor(0x374244),  // BLACK
        2.0f,
        0.0f
    });

    // Standard Pack - Red intense
    mPresets.push_back({
        "Standard Pack",
        HexToColor(0x272a2c),  // Darkened black
        HexToColor(0xFE5F55),  // RED as special color
        HexToColor(0x1a1c1d),  // Very dark
        3.0f,
        0.0f
    });

    // === BOSS BLINDS ===

    // The Ox - Orange
    mPresets.push_back({
        "The Ox",
        HexToColor(0xb95b08),
        HexToColor(0xe87a1c),  // Lighter (1.3x)
        HexToColor(0x4e2703),  // Mixed with black (0.4)
        2.0f,
        0.25f
    });

    // The Hook - Dark red
    mPresets.push_back({
        "The Hook",
        HexToColor(0xa84024),
        HexToColor(0xd85630),
        HexToColor(0x481b0f),
        2.0f,
        0.25f
    });

    // The Mouth - Pink/mauve
    mPresets.push_back({
        "The Mouth",
        HexToColor(0xae718e),
        HexToColor(0xd494b6),
        HexToColor(0x4a303c),
        2.0f,
        0.25f
    });

    // The Fish - Blue
    mPresets.push_back({
        "The Fish",
        HexToColor(0x3e85bd),
        HexToColor(0x5aabeb),
        HexToColor(0x1a3850),
        2.0f,
        0.25f
    });

    // The Club - Light green
    mPresets.push_back({
        "The Club",
        HexToColor(0xb9cb92),
        HexToColor(0xd4e6ad),
        HexToColor(0x4f573e),
        2.0f,
        0.25f
    });

    // The Manacle - Grey
    mPresets.push_back({
        "The Manacle",
        HexToColor(0x575757),
        HexToColor(0x717171),
        HexToColor(0x252525),
        2.0f,
        0.25f
    });

    // The Tooth - Blood red
    mPresets.push_back({
        "The Tooth",
        HexToColor(0xb52d2d),
        HexToColor(0xe54545),
        HexToColor(0x4d1313),
        2.0f,
        0.25f
    });

    // The Wall - Purple
    mPresets.push_back({
        "The Wall",
        HexToColor(0x8a59a5),
        HexToColor(0xb078ce),
        HexToColor(0x3a2646),
        2.0f,
        0.25f
    });

    // The House - Blue
    mPresets.push_back({
        "The House",
        HexToColor(0x5186a8),
        HexToColor(0x6db0d8),
        HexToColor(0x223947),
        2.0f,
        0.25f
    });

    // The Mark - Dark magenta
    mPresets.push_back({
        "The Mark",
        HexToColor(0x6a3847),
        HexToColor(0x8a4a5d),
        HexToColor(0x2d181e),
        2.0f,
        0.25f
    });

    // The Wheel - Bright green
    mPresets.push_back({
        "The Wheel",
        HexToColor(0x50bf7c),
        HexToColor(0x6ef4a0),
        HexToColor(0x225234),
        2.0f,
        0.25f
    });

    // The Arm - Purple-blue
    mPresets.push_back({
        "The Arm",
        HexToColor(0x6865f3),
        HexToColor(0x8a87ff),
        HexToColor(0x2c2b68),
        2.0f,
        0.25f
    });

    // The Psychic - Gold/yellow
    mPresets.push_back({
        "The Psychic",
        HexToColor(0xefc03c),
        HexToColor(0xffd85c),
        HexToColor(0x66521a),
        2.0f,
        0.25f
    });

    // The Goad - Pink
    mPresets.push_back({
        "The Goad",
        HexToColor(0xb95c96),
        HexToColor(0xe87ac0),
        HexToColor(0x4e2740),
        2.0f,
        0.25f
    });

    // The Water - Light blue/cyan
    mPresets.push_back({
        "The Water",
        HexToColor(0xc6e0eb),
        HexToColor(0xe8f5fc),
        HexToColor(0x546065),
        2.0f,
        0.25f
    });

    // The Eye - Blue
    mPresets.push_back({
        "The Eye",
        HexToColor(0x4b71e4),
        HexToColor(0x6d93ff),
        HexToColor(0x203062),
        2.0f,
        0.25f
    });

    // The Plant - Sage green
    mPresets.push_back({
        "The Plant",
        HexToColor(0x709284),
        HexToColor(0x92beac),
        HexToColor(0x303e38),
        2.0f,
        0.25f
    });

    // The Needle - Olive
    mPresets.push_back({
        "The Needle",
        HexToColor(0x5c6e31),
        HexToColor(0x788f40),
        HexToColor(0x272f15),
        2.0f,
        0.25f
    });

    // The Head - Lavender
    mPresets.push_back({
        "The Head",
        HexToColor(0xac9db4),
        HexToColor(0xd4c5dc),
        HexToColor(0x49434c),
        2.0f,
        0.25f
    });

    // The Window - Beige
    mPresets.push_back({
        "The Window",
        HexToColor(0xa9a295),
        HexToColor(0xd5ccc0),
        HexToColor(0x48453f),
        2.0f,
        0.25f
    });

    // The Serpent - Green
    mPresets.push_back({
        "The Serpent",
        HexToColor(0x439a4f),
        HexToColor(0x5cc76a),
        HexToColor(0x1c4221),
        2.0f,
        0.25f
    });

    // The Pillar - Brown
    mPresets.push_back({
        "The Pillar",
        HexToColor(0x7e6752),
        HexToColor(0xa3866b),
        HexToColor(0x352c23),
        2.0f,
        0.25f
    });

    // The Flint - Orange-red
    mPresets.push_back({
        "The Flint",
        HexToColor(0xe56a2f),
        HexToColor(0xff8a4f),
        HexToColor(0x622d14),
        2.0f,
        0.25f
    });

    // === SHOWDOWN BOSSES (special tertiary color scheme) ===

    // Cerulean Bell - Blue showdown
    mPresets.push_back({
        "Cerulean Bell",
        HexToColor(0x009cfd),  // Blue
        HexToColor(0xFE5F55),  // Red (special)
        HexToColor(0x272a2c),  // Dark tertiary
        3.0f,
        0.5f
    });

    // Verdant Leaf - Green showdown
    mPresets.push_back({
        "Verdant Leaf",
        HexToColor(0x56a786),
        HexToColor(0xFE5F55),
        HexToColor(0x272a2c),
        3.0f,
        0.5f
    });

    // Violet Vessel - Purple showdown
    mPresets.push_back({
        "Violet Vessel",
        HexToColor(0x8a71e1),
        HexToColor(0xFE5F55),
        HexToColor(0x272a2c),
        3.0f,
        0.5f
    });

    // Amber Acorn - Orange showdown
    mPresets.push_back({
        "Amber Acorn",
        HexToColor(0xfda200),
        HexToColor(0xFE5F55),
        HexToColor(0x272a2c),
        3.0f,
        0.5f
    });

    // Crimson Heart - Red showdown
    mPresets.push_back({
        "Crimson Heart",
        HexToColor(0xac3232),
        HexToColor(0x009dff),  // Blue contrast
        HexToColor(0x272a2c),
        3.0f,
        0.5f
    });

    // Set initial colors from first preset
    if (!mPresets.empty())
    {
        mColour1 = mPresets[0].colour1;
        mColour2 = mPresets[0].colour2;
        mColour3 = mPresets[0].colour3;
        mContrast = mPresets[0].contrast;
        mSpinAmount = mPresets[0].spinAmount;
    }
}

void BalatroEffect::Initialize(const std::filesystem::path& filePath)
{
    mVertexShader.Initialize<VertexPX>(filePath);
    mPixelShader.Initialize(filePath);
    mSampler.Initialize(Sampler::Filter::Point, Sampler::AddressMode::Wrap);
    mBalatroBuffer.Initialize();

    InitializePresets();
}

void BalatroEffect::Terminate()
{
    mBalatroBuffer.Terminate();
    mSampler.Terminate();
    mPixelShader.Terminate();
    mVertexShader.Terminate();
}

void BalatroEffect::SetPreset(int index, float transitionTime)
{
    if (index < 0 || index >= static_cast<int>(mPresets.size()))
        return;

    mCurrentPresetIndex = index;
    const auto& preset = mPresets[index];

    if (transitionTime <= 0.0f)
    {
        // Instant switch
        mColour1 = preset.colour1;
        mColour2 = preset.colour2;
        mColour3 = preset.colour3;
        mContrast = preset.contrast;
        mSpinAmount = preset.spinAmount;
        mIsTransitioning = false;
    }
    else
    {
        // Start smooth transition
        mStartColour1 = mColour1;
        mStartColour2 = mColour2;
        mStartColour3 = mColour3;
        mStartContrast = mContrast;
        mStartSpinAmount = mSpinAmount;

        mTargetColour1 = preset.colour1;
        mTargetColour2 = preset.colour2;
        mTargetColour3 = preset.colour3;
        mTargetContrast = preset.contrast;
        mTargetSpinAmount = preset.spinAmount;

        mTransitionTime = transitionTime;
        mTransitionProgress = 0.0f;
        mIsTransitioning = true;
    }
}

void BalatroEffect::SetPreset(const std::string& name, float transitionTime)
{
    for (int i = 0; i < static_cast<int>(mPresets.size()); ++i)
    {
        if (mPresets[i].name == name)
        {
            SetPreset(i, transitionTime);
            return;
        }
    }
}

void BalatroEffect::Update(float deltaTime)
{
    if (!mPaused)
    {
        mTime += deltaTime * mTimeSpeed;
        mSpinTime += deltaTime * mSpinSpeed;
    }

    // Handle transition
    if (mIsTransitioning)
    {
        mTransitionProgress += deltaTime / mTransitionTime;

        if (mTransitionProgress >= 1.0f)
        {
            mTransitionProgress = 1.0f;
            mIsTransitioning = false;
        }

        float t = Smoothstep(mTransitionProgress);

        mColour1 = LerpColor(mStartColour1, mTargetColour1, t);
        mColour2 = LerpColor(mStartColour2, mTargetColour2, t);
        mColour3 = LerpColor(mStartColour3, mTargetColour3, t);
        mContrast = mStartContrast + (mTargetContrast - mStartContrast) * t;
        mSpinAmount = mStartSpinAmount + (mTargetSpinAmount - mStartSpinAmount) * t;
    }
}

void BalatroEffect::Begin()
{
    mVertexShader.Bind();
    mPixelShader.Bind();
    mSampler.BindPS(0);

    GraphicsSystem* gs = GraphicsSystem::Get();

    BalatroData data;
    data.time = mTime;
    data.spinTime = mSpinTime;
    data.contrast = mContrast;
    data.spinAmount = mSpinAmount;
    data.colour1 = mColour1;
    data.colour2 = mColour2;
    data.colour3 = mColour3;
    data.screenSize.x = static_cast<float>(gs->GetBackBufferWidth());
    data.screenSize.y = static_cast<float>(gs->GetBackBufferHeight());
    data.pixelFilter = mPixelFilter;
    data.spinEase = mSpinEase;
    data.offset = mOffset;

    mBalatroBuffer.Update(data);
    mBalatroBuffer.BindPS(0);
}

void BalatroEffect::End()
{
    // Nothing to unbind
}

void BalatroEffect::Render(const RenderObject& renderObject)
{
    renderObject.meshBuffer.Render();
}

void BalatroEffect::DebugUI()
{
    // Preset selection
    if (ImGui::CollapsingHeader("Presets", ImGuiTreeNodeFlags_DefaultOpen))
    {
        static float transitionTime = 0.6f;

        ImGui::Separator();
        ImGui::Text("Game States:");

        // Game state presets (first 2)
        for (int i = 0; i < 2 && i < static_cast<int>(mPresets.size()); ++i)
        {
            bool isSelected = (mCurrentPresetIndex == i);
            if (isSelected)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));

            if (ImGui::Button(mPresets[i].name.c_str(), ImVec2(120, 0)))
            {
                SetPreset(i, transitionTime);
            }

            if (isSelected)
                ImGui::PopStyleColor();

            if ((i + 1) % 4 != 0 && i < 3)
                ImGui::SameLine();
        }

        ImGui::Separator();
        ImGui::Text("Booster Packs:");

        // Booster pack presets (next 5)
        for (int i = 2; i < 7 && i < static_cast<int>(mPresets.size()); ++i)
        {
            bool isSelected = (mCurrentPresetIndex == i);
            if (isSelected)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));

            if (ImGui::Button(mPresets[i].name.c_str(), ImVec2(120, 0)))
            {
                SetPreset(i, transitionTime);
            }

            if (isSelected)
                ImGui::PopStyleColor();

            if ((i - 2 + 1) % 3 != 0 && i < 6)
                ImGui::SameLine();
        }

        ImGui::Separator();
        ImGui::Text("Boss Blinds:");

        // Boss presets (rest)
        int bossCount = 0;
        for (int i = 7; i < static_cast<int>(mPresets.size()); ++i)
        {
            bool isSelected = (mCurrentPresetIndex == i);
            if (isSelected)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));

            // Truncate long boss names for button display
            std::string displayName = mPresets[i].name;
            if (displayName.length() > 16)
                displayName = displayName.substr(0, 16) + "..";

            if (ImGui::Button(displayName.c_str(), ImVec2(140, 0)))
            {
                SetPreset(i, transitionTime);
            }

            if (isSelected)
                ImGui::PopStyleColor();

            bossCount++;
            if (bossCount % 3 != 0)
                ImGui::SameLine();
        }
    }

    ImGui::Separator();

    // Manual controls
    if (ImGui::CollapsingHeader("Manual Controls"))
    {
        ImGui::Checkbox("Paused", &mPaused);
        ImGui::DragFloat("Time Speed", &mTimeSpeed, 0.1f, 0.0f, 20.0f);
        ImGui::DragFloat("Spin Speed", &mSpinSpeed, 0.1f, 0.0f, 10.0f);

        ImGui::Separator();

        ImGui::DragFloat("Contrast", &mContrast, 0.1f, 0.1f, 10.0f);
        ImGui::DragFloat("Spin Amount", &mSpinAmount, 0.01f, 0.0f, 2.0f);
        ImGui::DragFloat("Pixel Filter", &mPixelFilter, 10.0f, 100.0f, 2000.0f);

        ImGui::Separator();

        ImGui::ColorEdit4("Colour 1", &mColour1.r);
        ImGui::ColorEdit4("Colour 2", &mColour2.r);
        ImGui::ColorEdit4("Colour 3", &mColour3.r);

        ImGui::Separator();

        if (ImGui::Button("Reset Time"))
        {
            mTime = 0.0f;
            mSpinTime = 0.0f;
        }

        ImGui::SameLine();

        if (ImGui::Button("Reset All"))
        {
            mTime = 0.0f;
            mSpinTime = 0.0f;
            mContrast = 3.5f;
            mSpinAmount = 0.5f;
            mSpinEase = 0.5f;
            mPixelFilter = 700.0f;
            mTimeSpeed = 1.0f;
            mSpinSpeed = 1.0f;
            SetPreset(0, 0.0f);
        }
    }
}
