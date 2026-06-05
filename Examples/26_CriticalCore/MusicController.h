#pragma once

#include "CustomTypeIds.h"
#include "GmHelpers.h"

#include <Engine/Inc/Engine.h>

#include <string>

// Beat-clock / music controller for Critical Core 2 (port of objects/oMusicController).
//
// The whole game is synced to the 130-BPM music. Rather than a fixed timer, the
// beat is derived from the music PLAYBACK CURSOR (audio_sound_get_track_position):
//   currentBeat = cursorSeconds / 60 * 130
//   tick        = fmod(currentBeat, 0.5)            // half-beat phase
//   audioTick   = (tick < lastTick)                 // rising-edge on the half-beat wrap
//   audioBeat   = floor(fmod(currentBeat, 8) * 2) / 2   // quantized beat-in-bar, [0,7.5]
//
// POLICY (deliberate, user-chosen): per-fixed-step (60Hz) edge detection on the
// half-beat tick. If a frame is dropped under load the missed tick is simply lost
// — there is NO catch-up / replay. The cursor is the single source of truth; we
// never advance the beat off a fixed accumulator.
//
// THREAD-SAFETY: load + play the music handle ONCE, then only READ its cursor.
// Never Stop/Clear the handle while polling (the ma_sound lives on the audio
// thread) — see SoundEffectManager.h.
namespace Engine::CriticalCore
{
using Color = Engine::Graphics::Color;

//==================================================================================
// Pure beat math (shared by the production path AND the self-test).
//==================================================================================

// Result of advancing the beat one step from a cursor reading.
struct BeatStep
{
    bool audioTick = false;  // rising edge of the half-beat phase this step
    float audioBeat = 0.0f;  // quantized beat-in-bar, multiple of 0.5 in [0,7.5]
    bool barWrapped = false; // true on the step the bar rolls over (audioBeat decreased)
};

// Advances the beat from a cursor reading (seconds). lastTick/lastBar are the
// per-step persisted state (edge-detect + bar-wrap detect) and are updated in place.
// EXACTLY mirrors oMusicController/Step_0.gml:10-19.
BeatStep ComputeBeat(double cursorSeconds, float& lastTick, float& lastBar);

//==================================================================================
// BeatService — read interface the Core / Wall / flow components poll each step.
// The MusicControllerComponent owns the music + beat math and publishes here.
//==================================================================================
class BeatService final : public Engine::Service
{
  public:
    SET_TYPE_ID(CustomServiceId::BeatService);

    // Rising edge of the half-beat tick THIS fixed step (one-shot; cleared/rewritten
    // each step by the controller). Core shoots on this (task 21).
    bool AudioTick() const
    {
        return mAudioTick;
    }

    // Quantized beat-in-bar (multiple of 0.5, [0,7.5]).
    float AudioBeat() const
    {
        return mAudioBeat;
    }

    // Integer beat index (floor of audioBeat) — CoreShoot beatIndex argument.
    int BeatIndex() const
    {
        return static_cast<int>(mAudioBeat);
    }

    // Monotonic accumulator that drives the shCore shader iTime (task 21/24).
    float CoreEffectTime() const
    {
        return mCoreEffectTime;
    }

    // HSV pulse color chosen on each bar wrap (read by the Wall component, task 25).
    Color WallPulseColor() const
    {
        return mWallPulseColor;
    }

    // Pulse pattern selector (0 or 1) chosen on each bar wrap (task 25).
    int WallPulseType() const
    {
        return mWallPulseType;
    }

    // Published by MusicControllerComponent each fixed step.
    void Publish(bool audioTick, float audioBeat, float coreEffectTime);
    void SetWallPulse(const Color& color, int type);

  private:
    bool mAudioTick = false;
    float mAudioBeat = 0.0f;
    float mCoreEffectTime = 0.0f;
    Color mWallPulseColor = Engine::Graphics::Colors::Red;
    int mWallPulseType = 0;
};

//==================================================================================
// MusicControllerComponent — plays the looping music and drives the beat clock.
//==================================================================================
class MusicControllerComponent final : public Engine::Component
{
  public:
    SET_TYPE_ID(CustomComponentId::MusicControllerComponent);

    void Initialize() override;
    void Terminate() override;
    void Update(float deltaTime) override; // one call == one fixed step (kStep)
    void DebugUI() override;

    void Deserialize(const rapidjson::Value& value) override;

    // Direct accessors (mirror the BeatService getters) in case a reader holds the
    // component instead of the service.
    bool AudioTick() const
    {
        return mAudioTick;
    }
    float AudioBeat() const
    {
        return mAudioBeat;
    }
    int BeatIndex() const
    {
        return static_cast<int>(mAudioBeat);
    }
    float CoreEffectTime() const
    {
        return mCoreEffectTime;
    }
    Color WallPulseColor() const
    {
        return mWallPulseColor;
    }
    int WallPulseType() const
    {
        return mWallPulseType;
    }

  private:
    // Music handle — loaded + played ONCE, only its cursor is read afterwards.
    Engine::Audio::SoundId mMusicId = 0;
    bool mMusicStarted = false;
    std::string mMusicPath = "CriticalCore/mMusic.ogg"; // relative to Assets/Audio root

    // Persisted beat state (oMusicController instance vars).
    float mLastTick = 0.0f;
    float mLastBar = 0.0f;
    float mCoreEffectPulse = 0.0f;

    // Published beat state.
    bool mAudioTick = false;
    float mAudioBeat = 0.0f;
    float mCoreEffectTime = 0.0f;
    Color mWallPulseColor = Engine::Graphics::Colors::Red;
    int mWallPulseType = 0;

    BeatService* mBeatService = nullptr; // resolved in Initialize (may be null)
};

//==================================================================================
// Self-test: drives the beat math from a synthetic 10s cursor ramp at 60Hz and
// asserts the tick count and audioBeat quantization. Task 35 writes the CSV to
// .omo/evidence/task-18-beatlog.csv.
//==================================================================================
bool BeatSelfTest(std::string& csvOut);
} // namespace Engine::CriticalCore
