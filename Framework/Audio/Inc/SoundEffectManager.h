#pragma once

namespace Engine::Audio
{
using SoundId = std::size_t;

class SoundEffectManager final
{
  public:
    static void StaticInitialize();
    static void StaticTerminate();
    static SoundEffectManager* Get();

    SoundEffectManager() = default;
    ~SoundEffectManager();

    SoundEffectManager(const SoundEffectManager&) = delete;
    SoundEffectManager& operator=(const SoundEffectManager&) = delete;

    void SetRootPath(const std::filesystem::path& root);

    SoundId Load(const std::filesystem::path& fileName);
    void Clear();

    void Play(SoundId id, bool loop = false);
    void Stop(SoundId id);

    // THREAD-SAFETY: callers must never Stop/Clear a handle while another thread
    // polls its cursor — the ma_sound is read by the audio thread. The music
    // handle must stay alive for the whole game when its cursor is polled.
    float GetCursorSeconds(SoundId id) const;
    void SetVolume(SoundId id, float v);
    float GetVolume(SoundId id) const;

  private:
    std::unordered_map<SoundId, void*> mInventory;
    std::filesystem::path mRoot;
};
} // namespace Engine::Audio
