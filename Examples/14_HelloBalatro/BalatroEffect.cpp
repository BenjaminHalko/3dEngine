#include "BalatroEffect.h"

using namespace Engine;
using namespace Engine::Graphics;

void BalatroEffect::Initialize(const std::filesystem::path& filePath)
{
    mVertexShader.Initialize<VertexPX>(filePath);
    mPixelShader.Initialize(filePath);
    mSampler.Initialize(Sampler::Filter::Point, Sampler::AddressMode::Wrap);
    mBalatroBuffer.Initialize();
}

void BalatroEffect::Terminate()
{
    mBalatroBuffer.Terminate();
    mSampler.Terminate();
    mPixelShader.Terminate();
    mVertexShader.Terminate();
}

void BalatroEffect::Update(float deltaTime)
{
    if (!mPaused)
    {
        mTime += deltaTime * mTimeSpeed;
        mSpinTime += deltaTime * mSpinSpeed;
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
    if (ImGui::CollapsingHeader("Balatro Effect", ImGuiTreeNodeFlags_DefaultOpen))
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
        
        if (ImGui::Button("Reset Colors"))
        {
            mColour1 = { 0.871f, 0.267f, 0.231f, 1.0f };
            mColour2 = { 0.0f, 0.42f, 0.706f, 1.0f };
            mColour3 = { 0.086f, 0.137f, 0.145f, 1.0f };
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
            mColour1 = { 0.871f, 0.267f, 0.231f, 1.0f };
            mColour2 = { 0.0f, 0.42f, 0.706f, 1.0f };
            mColour3 = { 0.086f, 0.137f, 0.145f, 1.0f };
        }
    }
}
