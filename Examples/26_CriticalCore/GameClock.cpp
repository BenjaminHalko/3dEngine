#include "GameClock.h"

#include <array>
#include <string>

namespace Engine::CriticalCore
{
int GameClock::Advance(float deltaTime)
{
    mAccumulator += deltaTime;

    int steps = 0;
    while (mAccumulator >= kStep && steps < kMaxStepsPerFrame)
    {
        mAccumulator -= kStep;
        ++steps;
        ++mFrameCount;
        mTime += kStep;
    }
    return steps;
}

void GameClock::Reset()
{
    mAccumulator = 0.0f;
    mTime = 0.0f;
    mFrameCount = 0;
}

bool GameClock::SelfTest(std::string& csvOut)
{
    GameClock clock;

    // 2.0s of dt fed in IRREGULAR chunks (sums to exactly 2.0s).
    // Mix of sub-step, ~16.67ms, and larger frames to exercise the loop.
    const std::array<float, 40> chunks = {
        0.016f, 0.017f, 0.016f, 0.018f, 0.005f, 0.030f, 0.016f, 0.016f,
        0.001f, 0.050f, 0.016f, 0.016f, 0.016f, 0.016f, 0.020f, 0.012f,
        0.016f, 0.016f, 0.040f, 0.016f, 0.016f, 0.016f, 0.008f, 0.024f,
        0.016f, 0.016f, 0.016f, 0.016f, 0.016f, 0.016f, 0.060f, 0.016f,
        0.016f, 0.016f, 0.016f, 0.016f, 0.016f, 0.016f, 0.016f, 0.016f};

    // Normalize the chunk sum to exactly 2.0s so the expected count is deterministic.
    float rawSum = 0.0f;
    for (float c : chunks)
    {
        rawSum += c;
    }
    const float scale = 2.0f / rawSum;

    bool ok = true;
    int totalSteps = 0;

    csvOut = "frame,dt,accBefore,steps,frameCount,time,accAfter\n";
    int frameIndex = 0;
    for (float c : chunks)
    {
        const float dt = c * scale;
        const float accBefore = clock.Accumulator();
        const int steps = clock.Advance(dt);
        totalSteps += steps;

        // No step may run when the accumulator (after adding dt) was below kStep.
        if (steps == 0 && (accBefore + dt) >= kStep)
        {
            ok = false; // should have stepped but didn't
        }
        if (steps > 0 && (accBefore + dt) < kStep)
        {
            ok = false; // stepped while acc < kStep
        }

        csvOut += std::to_string(frameIndex) + ",";
        csvOut += std::to_string(dt) + ",";
        csvOut += std::to_string(accBefore) + ",";
        csvOut += std::to_string(steps) + ",";
        csvOut += std::to_string(clock.FrameCount()) + ",";
        csvOut += std::to_string(clock.Time()) + ",";
        csvOut += std::to_string(clock.Accumulator()) + "\n";
        ++frameIndex;
    }

    // 2.0s @ 60Hz == 120 steps; allow a small tolerance for fp accumulation.
    if (totalSteps < 118 || totalSteps > 122)
    {
        ok = false;
    }

    // FrameCount/Time consistency.
    if (clock.FrameCount() != static_cast<uint64_t>(totalSteps))
    {
        ok = false;
    }

    csvOut += "result," + std::string(ok ? "PASS" : "FAIL") + ",totalSteps,";
    csvOut += std::to_string(totalSteps) + ",finalTime,";
    csvOut += std::to_string(clock.Time()) + "\n";

    return ok;
}
} // namespace Engine::CriticalCore
