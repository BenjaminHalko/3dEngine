#pragma once

struct ma_engine;
struct ma_resource_manager;

namespace Engine::Audio
{
class AudioSystem final
{
  public:
    static void StaticInitialize();
    static void StaticTerminate();
    static AudioSystem* Get();

    AudioSystem() = default;
    ~AudioSystem();

    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;

    void Initialize();
    void Terminate();
    void Update();
    void Suspend();

    ma_engine* GetEngine()
    {
        return mAudioEngine;
    }

  private:
    friend class SoundEffectManager;
    ma_engine* mAudioEngine = nullptr;
    ma_resource_manager* mResourceManager = nullptr;
};
} // namespace Engine::Audio
