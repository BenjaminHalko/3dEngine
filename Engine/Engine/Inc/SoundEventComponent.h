#pragma once

#include "Component.h"

namespace Engine
{
class SoundEventComponent : public Component
{
  public:
    SET_TYPE_ID(ComponentId::SoundEffect);

    void Initialize() override;
    void Terminate() override;
    void DebugUI() override;
    void Deserialize(const rapidjson::Value& value) override;

    void Play();
    void Stop();

  private:
    std::string mFileName;
    bool mLooping = false;
    Audio::SoundId mSoundId = 0;
};
} // namespace Engine
