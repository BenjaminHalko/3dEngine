#include "CameraShakeService.h"

#include "GmHelpers.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace Engine::CriticalCore
{
namespace
{
// Follow constants (oCamera/Step_0.gml). The aim point is 35% of the way from
// the follow target toward the Core; the camera eases toward it with divisor 12.
constexpr float kFollowLerp = 0.35f; // lerp(target, core, 0.35)  (:5-6)
constexpr float kEaseDivisor = 12.0f; // x += (aim - x)/12         (:26-27)
} // namespace

void CameraShakeService::Initialize(int internalW, int internalH)
{
    mInternalWidth = internalW;
    mInternalHeight = internalH;

    // Snap to room centre (oCamera/Create_0.gml: x = room_width/2, y = room_height/2).
    mX = static_cast<float>(internalW) * 0.5f;
    mY = static_cast<float>(internalH) * 0.5f;

    mPlayerX = mX;
    mPlayerY = mY;
    mCoreX = mX;
    mCoreY = mY;

    mShakeMagnitude = 0.0f;
    mShakeRemain = 0.0f;
    mShakeLength = 0.0f;
    mShakeJitterX = 0.0f;
    mShakeJitterY = 0.0f;
}

void CameraShakeService::SetTargets(float playerX, float playerY, float coreX, float coreY)
{
    mPlayerX = playerX;
    mPlayerY = playerY;
    mCoreX = coreX;
    mCoreY = coreY;
}

void CameraShakeService::Update()
{
    // 1. Follow aim = floor(lerp(target, core, 0.35))   (Step_0.gml:5-6).
    const float aimX = std::floor(Math::Lerp(mPlayerX, mCoreX, kFollowLerp));
    const float aimY = std::floor(Math::Lerp(mPlayerY, mCoreY, kFollowLerp));

    // 2. Ease the camera centre toward the aim (Step_0.gml:26-27).
    mX += (aimX - mX) / kEaseDivisor;
    mY += (aimY - mY) / kEaseDivisor;

    // 3. Sample the shake jitter ONCE using the PRE-decay residual (Step_0.gml:30
    //    applies random_range(-shakeRemain, shakeRemain) on both axes before the
    //    line 31 decay). Sampling once keeps GetCameraOffset stable within a step
    //    and deterministic under a fixed RNG seed.
    mShakeJitterX = RandomRange(-mShakeRemain, mShakeRemain);
    mShakeJitterY = RandomRange(-mShakeRemain, mShakeRemain);

    // 4. Decay the residual linearly: remain -= magnitude/length, clamped >= 0
    //    (Step_0.gml:31). length guarded to avoid div-by-zero when idle.
    if (mShakeLength > 0.0f)
    {
        mShakeRemain = std::max(0.0f, mShakeRemain - (mShakeMagnitude / mShakeLength));
    }
    else
    {
        mShakeRemain = 0.0f;
    }
}

void CameraShakeService::ScreenShake(float magnitude, int frames)
{
    // Only override the in-flight shake if the new hit is stronger than what
    // remains (ScreenShake.gml:3-7). A smaller hit during a big shake is ignored.
    if (magnitude > mShakeRemain)
    {
        mShakeMagnitude = magnitude;
        mShakeRemain = magnitude;
        mShakeLength = static_cast<float>(frames);
    }
}

void CameraShakeService::GetCameraOffset(float& outX, float& outY) const
{
    // Camera top-left in 256x224 room space (x - viewW/2) plus this step's shake
    // jitter. This is the scene translation task 34 subtracts from world draw
    // origins (inside 256x224, before the point upscale).
    outX = (mX - static_cast<float>(mInternalWidth) * 0.5f) + mShakeJitterX;
    outY = (mY - static_cast<float>(mInternalHeight) * 0.5f) + mShakeJitterY;
}

void CameraShakeService::GetShake(float& outMag) const
{
    outMag = mShakeRemain;
}

bool CameraShakeService::ShakeSelfTest(std::string& csvOut)
{
    Initialize(256, 224);
    SeedRng(0xC0FFEEu); // deterministic jitter for reproducible --selftest

    // Seed a 12px shake over 40 fixed steps. Targets left at the room centre so
    // the follow base offset stays (0,0) and the measured offset is pure jitter.
    ScreenShake(12.0f, 40);

    std::ostringstream out;
    out << "step,envelope,jitterX,jitterY,offsetX,offsetY,offsetMag\n";

    constexpr int kSteps = 45;
    constexpr float kPeak = 12.0f;
    const float kSqrt2 = std::sqrt(2.0f);

    float peak = -1.0f;
    float prevEnvelope = 1.0e30f;
    bool monotonic = true;
    bool bounded = true;
    int zeroStep = -1;

    for (int step = 0; step < kSteps; ++step)
    {
        // Envelope = residual that drives THIS step's jitter (pre-decay).
        float envelope = 0.0f;
        GetShake(envelope);

        Update(); // samples jitter from envelope, then decays the residual

        float ox = 0.0f;
        float oy = 0.0f;
        GetCameraOffset(ox, oy);
        const float mag = std::sqrt(ox * ox + oy * oy);

        out << step << ',' << envelope << ',' << mShakeJitterX << ',' << mShakeJitterY << ',' << ox << ',' << oy << ','
            << mag << '\n';

        peak = std::max(peak, envelope);

        // Envelope must never grow step-to-step (monotonic decay).
        if (envelope > prevEnvelope + 1.0e-4f)
        {
            monotonic = false;
        }
        prevEnvelope = envelope;

        // Per-axis jitter is bounded by the envelope, so |offset| <= sqrt(2)*env.
        if (mag > kSqrt2 * envelope + 1.0e-3f)
        {
            bounded = false;
        }

        if (zeroStep < 0 && envelope <= 1.0e-4f)
        {
            zeroStep = step;
        }
    }

    const bool peakOk = std::fabs(peak - kPeak) <= 0.5f;
    const bool decaysToZero = (zeroStep >= 0 && zeroStep <= 41);
    const bool pass = peakOk && monotonic && bounded && decaysToZero;

    out << "RESULT,peak=" << peak << ",peakOk=" << (peakOk ? 1 : 0) << ",monotonic=" << (monotonic ? 1 : 0)
        << ",bounded=" << (bounded ? 1 : 0) << ",zeroStep=" << zeroStep << ",decaysToZero=" << (decaysToZero ? 1 : 0)
        << ",pass=" << (pass ? 1 : 0) << '\n';

    csvOut = out.str();
    return pass;
}
} // namespace Engine::CriticalCore
