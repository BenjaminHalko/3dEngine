#include "Precompiled.h"
#include "AudioSystem.h"

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
    mAudioEngine = new ma_engine();
    ma_result result = ma_engine_init(NULL, mAudioEngine);
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
}

void AudioSystem::Update()
{
    // miniaudio handles updates internally
}

void AudioSystem::Suspend()
{
    // miniaudio handles this automatically
}
