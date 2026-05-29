#pragma once

#include "Component.h"

namespace Engine
{
class SoundBankComponent : public Component
{
  public:
    SET_TYPE_ID(ComponentId::SoundBank);

    void Initialize() override;
    void Terminate() override;
    void DebugUI() override;
    void Deserialize(const rapidjson::Value& value) override;

    void Play(const std::string& key);
    void Stop(const std::string& key);
    void StopAll();

  private:
    struct SoundEffectData
    {
        std::string fileName;
        Audio::SoundId soundId = 0;
        bool looping = false;
    };

    using SoundEffects = std::unordered_map<std::string, SoundEffectData>;
    SoundEffects mSoundEffects;
};
} // namespace Engine
