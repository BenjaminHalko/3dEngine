#include "MusicController.h"

#include "GameClock.h"

#include <cmath>
#include <sstream>

namespace Engine::CriticalCore
{
//==================================================================================
// Pure beat math — single source of truth (production + self-test).
//==================================================================================
BeatStep ComputeBeat(double cursorSeconds, float& lastTick, float& lastBar)
{
    // oMusicController/Step_0.gml:10-19
    const double currentBeat = cursorSeconds / 60.0 * 130.0;
    const float tick = static_cast<float>(std::fmod(currentBeat, 0.5));

    BeatStep out;
    out.audioTick = (tick < lastTick); // rising edge of the half-beat phase
    out.audioBeat = static_cast<float>(std::floor(std::fmod(currentBeat, 8.0) * 2.0) / 2.0);
    lastTick = tick;

    out.barWrapped = (lastBar > out.audioBeat); // bar rolled over
    lastBar = out.audioBeat;
    return out;
}

//==================================================================================
// BeatService
//==================================================================================
void BeatService::Publish(bool audioTick, float audioBeat, float coreEffectTime)
{
    mAudioTick = audioTick;
    mAudioBeat = audioBeat;
    mCoreEffectTime = coreEffectTime;
}

void BeatService::SetWallPulse(const Color& color, int type)
{
    mWallPulseColor = color;
    mWallPulseType = type;
}

//==================================================================================
// MusicControllerComponent
//==================================================================================
void MusicControllerComponent::Initialize()
{
    // Resolve the shared BeatService (added pre-Initialize). May be null if the
    // level didn't register it — the component still works standalone.
    mBeatService = GetOwner().GetWorld().GetService<BeatService>();

    // Load + play the looping music ONCE. After this we only read the cursor —
    // never Stop/Clear while polling (audio-thread race, SoundEffectManager.h).
    Engine::Audio::SoundEffectManager* sm = Engine::Audio::SoundEffectManager::Get();
    mMusicId = sm->Load(mMusicPath);
    sm->Play(mMusicId, /*loop=*/true);
    mMusicStarted = true;

    // oMusicController/Create_0.gml initial state.
    mLastTick = 0.0f;
    mLastBar = 0.0f;
    mCoreEffectPulse = 0.0f;
    mAudioTick = false;
    mAudioBeat = 0.0f;
    mCoreEffectTime = 0.0f;
    mWallPulseColor = Engine::Graphics::Colors::Red;
    mWallPulseType = 0;

    if (mBeatService != nullptr)
    {
        mBeatService->Publish(mAudioTick, mAudioBeat, mCoreEffectTime);
        mBeatService->SetWallPulse(mWallPulseColor, mWallPulseType);
    }
}

void MusicControllerComponent::Terminate()
{
    // Stop the handle only at shutdown, once polling has ceased.
    if (mMusicStarted)
    {
        Engine::Audio::SoundEffectManager::Get()->Stop(mMusicId);
        mMusicStarted = false;
    }
    mBeatService = nullptr;
}

void MusicControllerComponent::Update(float deltaTime)
{
    (void)deltaTime; // beat is driven by the music CURSOR, never a timer

    // Read the music PLAYBACK CURSOR (advances for the OGG even though length()==0;
    // confirmed task 7/vorbis). This is the single source of truth for the beat.
    const double cursor =
        static_cast<double>(Engine::Audio::SoundEffectManager::Get()->GetCursorSeconds(mMusicId));

    // Per-step rising-edge beat math. No catch-up: a dropped frame just loses a tick.
    const BeatStep step = ComputeBeat(cursor, mLastTick, mLastBar);
    mAudioTick = step.audioTick;
    mAudioBeat = step.audioBeat;

    // On a bar wrap pick a fresh HSV wall-pulse color + pattern type
    // (oMusicController/Step_0.gml:15-18).
    if (step.barWrapped)
    {
        mWallPulseColor = MakeColorHSV(RandomRange(0.0f, 255.0f), 255.0f, 255.0f);
        mWallPulseType = IRandom(1);
        if (mBeatService != nullptr)
        {
            mBeatService->SetWallPulse(mWallPulseColor, mWallPulseType);
        }
    }

    // Core-effect pulse: kick to 1 on a beat tick, then ease back; the time
    // accumulates every step and feeds the shCore iTime
    // (oMusicController/Step_0.gml:44-60).
    const bool onBeat = (std::fmod(mAudioBeat, 1.0f) == 0.0f);
    if (mAudioTick && onBeat)
    {
        mCoreEffectPulse = 1.0f;
    }
    mCoreEffectPulse = ApproachFade(mCoreEffectPulse, 0.0f, 0.1f, 0.8f);
    mCoreEffectTime += 0.005f + mCoreEffectPulse * 0.02f;

    if (mBeatService != nullptr)
    {
        mBeatService->Publish(mAudioTick, mAudioBeat, mCoreEffectTime);
    }
}

void MusicControllerComponent::DebugUI()
{
    ImGui::Text("Beat: %.1f (tick=%d)", mAudioBeat, mAudioTick ? 1 : 0);
    ImGui::Text("CoreEffectTime: %.3f", mCoreEffectTime);
    ImGui::Text("WallPulseType: %d", mWallPulseType);
}

void MusicControllerComponent::Deserialize(const rapidjson::Value& value)
{
    if (value.HasMember("musicPath"))
    {
        mMusicPath = value["musicPath"].GetString();
    }
}

//==================================================================================
// Self-test: synthetic 60Hz cursor ramp for ~10s through the production beat math.
//==================================================================================
bool BeatSelfTest(std::string& csvOut)
{
    std::ostringstream csv;
    csv << "frame,time,cursor,beat,tick,audioTick\n";

    constexpr float kStep = GameClock::kStep; // 1/60 s
    constexpr float kElapsed = 10.0f;
    const int totalSteps = static_cast<int>(kElapsed / kStep); // 600

    float lastTick = 0.0f;
    float lastBar = 0.0f;
    int tickCount = 0;
    bool beatQuantOk = true;

    for (int i = 0; i < totalSteps; ++i)
    {
        // Synthetic cursor ramps linearly with the fixed clock (the production path
        // reads the real ma_sound cursor instead).
        const double cursor = static_cast<double>(i) * static_cast<double>(kStep);
        const float currentBeat = static_cast<float>(cursor / 60.0 * 130.0);
        const float tickPhase = static_cast<float>(std::fmod(cursor / 60.0 * 130.0, 0.5));

        const BeatStep step = ComputeBeat(cursor, lastTick, lastBar);
        if (step.audioTick)
        {
            ++tickCount;
        }

        // audioBeat must be a multiple of 0.5 in [0, 7.5].
        const float twice = step.audioBeat * 2.0f;
        if (std::fabs(twice - std::round(twice)) > 1e-4f || step.audioBeat < 0.0f ||
            step.audioBeat > 7.5f)
        {
            beatQuantOk = false;
        }

        csv << i << ',' << (i * kStep) << ',' << cursor << ',' << currentBeat << ',' << tickPhase
            << ',' << (step.audioTick ? 1 : 0) << '\n';
    }

    // Expected number of half-beat ticks over the elapsed window:
    //   floor(elapsed * 130 / 60 / 0.5)
    const int expectedTicks = static_cast<int>(std::floor(kElapsed * 130.0 / 60.0 / 0.5));
    const bool tickCountOk = std::abs(tickCount - expectedTicks) <= 1;

    const bool pass = tickCountOk && beatQuantOk;
    csv << "result," << (pass ? "PASS" : "FAIL") << ",ticks=" << tickCount
        << ",expected=" << expectedTicks << ",quant=" << (beatQuantOk ? "ok" : "bad") << ",,\n";

    csvOut = csv.str();
    return pass;
}
} // namespace Engine::CriticalCore
