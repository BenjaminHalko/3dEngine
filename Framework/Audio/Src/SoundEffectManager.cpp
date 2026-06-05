#include "Precompiled.h"
#include "SoundEffectManager.h"
#include "AudioSystem.h"

#include <miniaudio/miniaudio.h>

using namespace Engine;
using namespace Engine::Audio;

namespace
{
std::unique_ptr<SoundEffectManager> sSoundEffectManager;
}

void SoundEffectManager::StaticInitialize()
{
    ASSERT(sSoundEffectManager == nullptr, "SoundEffectManager: Is already initialized!");
    sSoundEffectManager = std::make_unique<SoundEffectManager>();
}

void SoundEffectManager::StaticTerminate()
{
    if (sSoundEffectManager != nullptr)
    {
        sSoundEffectManager->Clear();
        sSoundEffectManager.reset();
    }
}

SoundEffectManager* SoundEffectManager::Get()
{
    ASSERT(sSoundEffectManager != nullptr, "SoundEffectManager: Is not initialized!");
    return sSoundEffectManager.get();
}

SoundEffectManager::~SoundEffectManager()
{
    ASSERT(mInventory.empty(), "SoundEffectManager: Isn't Terminated!");
}

void SoundEffectManager::SetRootPath(const std::filesystem::path& root)
{
    mRoot = root;
}

SoundId SoundEffectManager::Load(const std::filesystem::path& fileName)
{
    std::filesystem::path fullPath = mRoot / fileName;
    std::size_t soundId = std::filesystem::hash_value(fullPath);

    auto iter = mInventory.find(soundId);
    if (iter == mInventory.end())
    {
        ma_sound* sound = new ma_sound();
        ma_result result = ma_sound_init_from_file(
            AudioSystem::Get()->GetEngine(), fullPath.string().c_str(), 0, NULL, NULL, sound);
        if (result == MA_SUCCESS)
        {
            mInventory[soundId] = sound;
        }
        else
        {
            LOG("SoundEffectManager: Failed to load: %s", fullPath.string().c_str());
            delete sound;
            return 0;
        }
    }
    return soundId;
}

SoundId SoundEffectManager::Load(const std::filesystem::path& fileName, float baseVolume)
{
    SoundId id = Load(fileName);
    if (id != 0)
    {
        SetVolume(id, baseVolume);
    }
    return id;
}

void SoundEffectManager::Clear()
{
    for (auto& [id, soundPtr] : mInventory)
    {
        if (soundPtr != nullptr)
        {
            ma_sound* sound = static_cast<ma_sound*>(soundPtr);
            ma_sound_stop(sound);
            ma_sound_uninit(sound);
            delete sound;
        }
    }
    mInventory.clear();
}

void SoundEffectManager::Play(SoundId id, bool loop)
{
    auto iter = mInventory.find(id);
    if (iter != mInventory.end() && iter->second != nullptr)
    {
        ma_sound* sound = static_cast<ma_sound*>(iter->second);
        ma_sound_set_looping(sound, loop ? MA_TRUE : MA_FALSE);
        ma_sound_start(sound);
    }
}

void SoundEffectManager::Stop(SoundId id)
{
    auto iter = mInventory.find(id);
    if (iter != mInventory.end() && iter->second != nullptr)
    {
        ma_sound* sound = static_cast<ma_sound*>(iter->second);
        ma_sound_stop(sound);
    }
}

float SoundEffectManager::GetCursorSeconds(SoundId id) const
{
    auto iter = mInventory.find(id);
    if (iter != mInventory.end() && iter->second != nullptr)
    {
        ma_sound* sound = static_cast<ma_sound*>(iter->second);
        float cursor = 0.0f;
        if (ma_sound_get_cursor_in_seconds(sound, &cursor) == MA_SUCCESS)
        {
            return cursor;
        }
    }
    return 0.0f;
}

void SoundEffectManager::SetVolume(SoundId id, float v)
{
    auto iter = mInventory.find(id);
    if (iter != mInventory.end() && iter->second != nullptr)
    {
        ma_sound* sound = static_cast<ma_sound*>(iter->second);
        ma_sound_set_volume(sound, v);
    }
}

float SoundEffectManager::GetVolume(SoundId id) const
{
    auto iter = mInventory.find(id);
    if (iter != mInventory.end() && iter->second != nullptr)
    {
        ma_sound* sound = static_cast<ma_sound*>(iter->second);
        return ma_sound_get_volume(sound);
    }
    return 0.0f;
}

void SoundEffectManager::SetPitch(SoundId id, float pitch)
{
    auto iter = mInventory.find(id);
    if (iter != mInventory.end() && iter->second != nullptr)
    {
        ma_sound* sound = static_cast<ma_sound*>(iter->second);
        ma_sound_set_pitch(sound, pitch);
    }
}

float SoundEffectManager::GetPitch(SoundId id) const
{
    auto iter = mInventory.find(id);
    if (iter != mInventory.end() && iter->second != nullptr)
    {
        ma_sound* sound = static_cast<ma_sound*>(iter->second);
        return ma_sound_get_pitch(sound);
    }
    return 0.0f;
}
