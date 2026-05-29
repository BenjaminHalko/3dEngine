#include "Precompiled.h"
#include "SoundBankComponent.h"
#include "SaveUtil.h"

using namespace Engine;
using namespace Engine::Audio;

void SoundBankComponent::Initialize()
{
    SoundEffectManager* sem = SoundEffectManager::Get();
    for (auto& data : mSoundEffects)
    {
        data.second.soundId = sem->Load(data.second.fileName);
    }
}

void SoundBankComponent::Terminate()
{
    StopAll();
}

void SoundBankComponent::DebugUI()
{
    if (ImGui::CollapsingHeader("SoundBank"))
    {
        ImGui::Indent();
        for (auto& data : mSoundEffects)
        {
            ImGui::PushID(data.first.c_str());
            ImGui::Text("%s", data.first.c_str());
            ImGui::Text("%s", data.second.fileName.c_str());
            if (ImGui::Button("Play"))
            {
                Play(data.first);
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop"))
            {
                Stop(data.first);
            }
            ImGui::PopID();
        }
        if (ImGui::Button("StopAll"))
        {
            StopAll();
        }
        ImGui::Unindent();
    }
}

void SoundBankComponent::Deserialize(const rapidjson::Value& value)
{
    if (value.HasMember("SoundEffects"))
    {
        auto soundEffects = value["SoundEffects"].GetObj();
        for (auto& effect : soundEffects)
        {
            SoundEffectData& data = mSoundEffects[effect.name.GetString()];
            SaveUtil::ReadString("FileName", data.fileName, effect.value);
            SaveUtil::ReadBool("Looping", data.looping, effect.value);
        }
    }
}

void SoundBankComponent::Play(const std::string& key)
{
    auto iter = mSoundEffects.find(key);
    if (iter != mSoundEffects.end())
    {
        SoundEffectManager::Get()->Play(iter->second.soundId, iter->second.looping);
    }
}

void SoundBankComponent::Stop(const std::string& key)
{
    auto iter = mSoundEffects.find(key);
    if (iter != mSoundEffects.end())
    {
        SoundEffectManager::Get()->Stop(iter->second.soundId);
    }
}

void SoundBankComponent::StopAll()
{
    SoundEffectManager* sem = SoundEffectManager::Get();
    for (auto& data : mSoundEffects)
    {
        sem->Stop(data.second.soundId);
    }
}
