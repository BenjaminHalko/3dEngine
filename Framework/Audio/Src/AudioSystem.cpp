#include "Precompiled.h"
#include "AudioSystem.h"
#include "VorbisDecoder.h"

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio/miniaudio.h>

using namespace Engine;
using namespace Engine::Audio;

namespace
{
std::unique_ptr<AudioSystem> sAudioSystem;
}

void AudioSystem::StaticInitialize()
{
    ASSERT(sAudioSystem == nullptr, "AudioSystem: System already initialized!");
    sAudioSystem = std::make_unique<AudioSystem>();
    sAudioSystem->Initialize();
}

void AudioSystem::StaticTerminate()
{
    if (sAudioSystem != nullptr)
    {
        sAudioSystem->Terminate();
        sAudioSystem.reset();
    }
}

AudioSystem* AudioSystem::Get()
{
    ASSERT(sAudioSystem != nullptr, "AudioSystem: Is not Initialized!");
    return sAudioSystem.get();
}

AudioSystem::~AudioSystem()
{
    ASSERT(mAudioEngine == nullptr, "AudioSystem: Must call Terminate!");
}

void AudioSystem::Initialize()
{
    // Register the custom OGG/Vorbis decoding backend (stb_vorbis) with a
    // resource manager so EVERY sound loaded through the engine can decode
    // .ogg files. The backend is ADDITIVE: WAV/FLAC/MP3 still use miniaudio's
    // built-ins; Vorbis goes through g_ma_decoding_backend_vtable_stbvorbis.
    static ma_decoding_backend_vtable* customBackends[] = {
        &g_ma_decoding_backend_vtable_stbvorbis,
    };

    mResourceManager = new ma_resource_manager();
    ma_resource_manager_config resourceManagerConfig = ma_resource_manager_config_init();
    resourceManagerConfig.ppCustomDecodingBackendVTables = customBackends;
    resourceManagerConfig.customDecodingBackendCount = 1;
    resourceManagerConfig.pCustomDecodingBackendUserData = NULL;

    ma_result result = ma_resource_manager_init(&resourceManagerConfig, mResourceManager);
    ASSERT(result == MA_SUCCESS, "AudioSystem: Failed to initialize miniaudio resource manager!");

    mAudioEngine = new ma_engine();
    ma_engine_config engineConfig = ma_engine_config_init();
    engineConfig.pResourceManager = mResourceManager;

    result = ma_engine_init(&engineConfig, mAudioEngine);
    ASSERT(result == MA_SUCCESS, "AudioSystem: Failed to initialize miniaudio engine!");
}

void AudioSystem::Terminate()
{
    if (mAudioEngine != nullptr)
    {
        ma_engine_uninit(mAudioEngine);
        delete mAudioEngine;
        mAudioEngine = nullptr;
    }

    if (mResourceManager != nullptr)
    {
        ma_resource_manager_uninit(mResourceManager);
        delete mResourceManager;
        mResourceManager = nullptr;
    }
}

void AudioSystem::Update()
{
    // miniaudio handles updates internally
}

void AudioSystem::Suspend()
{
    // miniaudio handles this automatically
}
