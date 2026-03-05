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

  private:
    std::unordered_map<SoundId, void*> mInventory;
    std::filesystem::path mRoot;
};
} // namespace Engine::Audio
