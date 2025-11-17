#include "Precompiled.h"
#include "PostProcessingEffect.h"

#include "RenderObject.h"
#include "Texture.h"
#include "VertexTypes.h"
#include "GraphicsSystem.h"

using namespace Engine;
using namespace Engine::Graphics;

namespace 
{
    const char* gModeNames[] =
    {
    "None",
    "Monochrome",
    "Invert",
    "Mirror",
    "Blur",
    "Combine2",
    "MotionBlur",
    "ChromaticAberration",
    "Wave",
    };
}


void PostProcessingEffect::Initialize(const std::filesystem::path& filePath)
{    
    mVertexShader.Initialize<VertexPX>(filePath);
    mPixelShader.Initialize(filePath);
    mSampler.Initialize(Sampler::Filter::Point, Sampler::AddressMode::Wrap);
    mPostProcessBuffer.Initialize();
}    
     
void PostProcessingEffect::Terminate()
{   
    mPostProcessBuffer.Terminate();
    mSampler.Terminate();
    mPixelShader.Terminate();
    mVertexShader.Terminate();
}    
     
void PostProcessingEffect::Begin()
{  
    mVertexShader.Bind();
    mPixelShader.Bind();
    mSampler.BindPS(0);

    for (uint32_t i = 0; i < mTextures.size(); ++i)
    {
        if (mTextures[i] != nullptr)
        {
            mTextures[i]->BindPS(i);
        }
    }

    PostProcessData data;
    data.mode = static_cast<int>(mMode);

    switch (mMode)
    {
    case Mode::None:
    case Mode::Monochrome:
    case Mode::Invert:
        break;
    case Mode::Mirror:
    {
        data.param0 = mMirrorScaleX;
        data.param1 = mMirrorScaleY;
    }
    break;
    case Mode::Blur:
    case Mode::MotionBlur:
    {
        GraphicsSystem* gs = GraphicsSystem::Get();
        const float screenWidth = gs->GetBackBufferWidth();
        const float screenHeight = gs->GetBackBufferHeight();
        data.param0 = mBlurStrength / screenWidth;
        data.param1 = mBlurStrength / screenHeight;
    }
        break;
    case Mode::Combine2:
    {
        data.param0 = mCombine2Alpha;
    }
    break;
    case Mode::ChromaticAberration:
    {
        data.param0 = mAberrationValue;
        data.param1 = mAberrationValue;
    }
    break;
    case Mode::Wave:
    {
        data.param0 = mWaveLenght;
        data.param1 = mNumWaves;
    }
    default:
        break;
    }

    mPostProcessBuffer.Update(data);
    mPostProcessBuffer.BindPS(0);
}    
     
void PostProcessingEffect::End()
{ 
    for (uint32_t i = 0; i < mTextures.size(); ++i)
    {
        Texture::UnbindPS(i);
    }
}    
     
void PostProcessingEffect::Render(const RenderObject& renderObject)
{  
    renderObject.meshBuffer.Render();
}    
     
void PostProcessingEffect::SetTexture(const Texture* texture, uint32_t slot)
{    
    ASSERT(slot < mTextures.size(), "PostProcessingEffect: Invalid slot index!");
    mTextures[slot] = texture;
}

void PostProcessingEffect::SetMode(Mode mode)
{
    mMode = mode;

}
     
void PostProcessingEffect::DebugUI()
{
    if (ImGui::CollapsingHeader("PostProcessingEffect", ImGuiTreeNodeFlags_DefaultOpen))
    {
        int currentMode = static_cast<int>(mMode);
        if (ImGui::Combo("Mode", &currentMode, gModeNames, std::size(gModeNames)))
        {
            mMode = static_cast<Mode>(currentMode);
        }
        if (mMode == Mode::Mirror)
        {
            ImGui::DragFloat("MirrorScaleX", &mMirrorScaleX, 0.1f, -1.0f, 1.0f);
            ImGui::DragFloat("MirrorScaleY", &mMirrorScaleY, 0.1f, -1.0f, 1.0f);
        }
        else if (mMode == Mode::Blur || mMode == Mode::MotionBlur)
        {
            ImGui::DragFloat("BlurStrenght", &mBlurStrength, 0.1f, 0.0f, 100.0f);
        }
        else if (mMode == Mode::Combine2)
        {
            ImGui::DragFloat("Combine2Alpha", &mCombine2Alpha, 0.01f, 0.0f, 1.0f);
        }
        else if (mMode == Mode::ChromaticAberration)
        {
            ImGui::DragFloat("AberrationAmount", &mAberrationValue, 0.001f, 0.001f, 1.0f);
        }
        else if (mMode == Mode::Wave)
        {
            ImGui::DragFloat("WaveLength", &mWaveLenght, 0.001f, 0.0f, 1.0f);
            ImGui::DragFloat("NumWave", &mNumWaves, 0.0f, 1.0f, 1000.0f);
        }
    }
}


