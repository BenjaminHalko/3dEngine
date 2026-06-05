#pragma once

#include <cstdint>
#include <string>

namespace Engine::CriticalCore
{
// Fixed 60Hz timestep accumulator + authoritative game clock.
// Logic runs in fixed 1/60s steps; render is locked to logic ticks (no interpolation).
// This is the single clock that drives Update/Wave/beat.
class GameClock
{
public:
    static constexpr float kStep = 1.0f / 60.0f;
    static constexpr int kMaxStepsPerFrame = 5; // cap to avoid spiral-of-death

    // Adds deltaTime to the accumulator and returns how many fixed steps to run this frame.
    // Capped at kMaxStepsPerFrame to survive a long stall; advances FrameCount/Time per step.
    int Advance(float deltaTime);

    // Total fixed steps executed since construction/Reset.
    uint64_t FrameCount() const { return mFrameCount; }

    // Authoritative game time in seconds, derived from frameCount * kStep (monotonic).
    // Use this for Wave()/beat math, never wall-clock time.
    float Time() const { return mTime; }

    // Leftover accumulator (for debugging only).
    float Accumulator() const { return mAccumulator; }

    void Reset();

    // Feeds 2.0s of dt in irregular chunks; asserts step count in [118,122]
    // and that no step runs while acc < kStep. Writes a CSV trace to csvOut.
    static bool SelfTest(std::string& csvOut);

private:
    float mAccumulator = 0.0f;
    float mTime = 0.0f;
    uint64_t mFrameCount = 0;
};
} // namespace Engine::CriticalCore
