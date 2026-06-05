#include "BackgroundComponent.h"

#include "CriticalCore2DRenderService.h"
#include "GmHelpers.h"
#include "Render2D.h"

using namespace Engine;

namespace
{
// Arena centre = room_width/2, room_height/2 (256x224).
constexpr float kCenterX = 128.0f;
constexpr float kCenterY = 112.0f;
constexpr float kRoomWidth = 256.0f;

// One fixed step = 1/60 s. Logic is fixed-step (learnings: never scale by dt).
constexpr float kStep = 1.0f / 60.0f;

// sBGBubbles origin (3,3) per the task-2 sprite manifest.
constexpr float kBubbleOriginX = 3.0f;
constexpr float kBubbleOriginY = 3.0f;
} // namespace

namespace Engine::CriticalCore
{
void BackgroundComponent::Initialize()
{
    // Base caches transform + registers with the render service.
    Render2DComponent::Initialize();

    CriticalCore2DRenderService* service = GetRenderService();
    if (service != nullptr)
    {
        Render2D& render2D = service->GetRender2D();
        for (int i = 0; i < kFrameCount; ++i)
        {
            mFrames[i] = render2D.LoadTexture("CriticalCore/sBGBubbles_" + std::to_string(i) + ".png");
        }
        mLoaded = true;
    }

    SpawnBubbles();
}

void BackgroundComponent::SpawnBubbles()
{
    // oBackground/Create_0.gml: scatter bubbleCount bubbles within room_width of
    // the centre, random heading, frame and zeroed motion/alpha.
    mBubbles.clear();
    if (mBubbleCount < 0)
    {
        mBubbleCount = 0;
    }
    mBubbles.reserve(static_cast<size_t>(mBubbleCount));
    for (int i = 0; i < mBubbleCount; ++i)
    {
        const float dir = RandomRange(0.0f, 360.0f);
        const float dist = RandomRange(0.0f, kRoomWidth);

        Bubble b;
        b.index = IRandom(kFrameCount - 1);
        b.x = kCenterX + LengthDirX(dist, dir);
        b.y = kCenterY + LengthDirY(dist, dir);
        b.dir = dir;
        b.spd = 0.0f;
        b.alpha = 0.0f;
        b.purple = 0.0f;
        mBubbles.push_back(b);
    }
}

void BackgroundComponent::Update(float deltaTime)
{
    (void)deltaTime; // fixed-step: one Update == one GameMaker step.

    // oBackground/Step_0.gml (simplified: no oCore / oBubble interaction; drift
    // is relative to the arena centre, targets are the source's idle values).
    for (Bubble& b : mBubbles)
    {
        b.dir = PointDirection(kCenterX, kCenterY, b.x, b.y);

        b.x += LengthDirX(b.spd, b.dir);
        b.y += LengthDirY(b.spd, b.dir);
        b.spd = ApproachFade(b.spd, 0.15f, 0.1f, 0.8f);
        b.alpha = ApproachFade(b.alpha, 0.5f, 0.05f, 0.8f);
        b.purple = ApproachFade(b.purple, 0.0f, 0.05f, 0.8f);

        // Wrap back toward the centre once it drifts past the room edge.
        if (PointDistance(b.x, b.y, kCenterX, kCenterY) > kRoomWidth)
        {
            const float dir = RandomRange(0.0f, 360.0f);
            const float dist = RandomRange(0.0f, kRoomWidth / 4.0f);
            b.x = kCenterX + LengthDirX(dist, dir);
            b.y = kCenterY + LengthDirY(dist, dir);
            b.alpha = 0.0f;
            b.purple = 0.0f;
        }
    }

    mTime += kStep;

    // oBackground/Step_0.gml: bgFlash = Approach(bgFlash, 0, 0.2).
    mBgFlash = Approach(mBgFlash, 0.0f, 0.2f);
}

void BackgroundComponent::Draw(Render2D& render2D)
{
    if (!mLoaded || mBubbles.empty())
    {
        return;
    }

    // merge_color endpoints from oBackground/Draw_0.gml.
    const Color baseTint(0.29804f, 0.29804f, 0.35294f, 1.0f); // #4C4C5A
    const Color fuchsia(1.0f, 0.0f, 1.0f, 1.0f);              // c_fuchsia

    const float count = static_cast<float>(mBubbleCount);
    for (int i = 0; i < static_cast<int>(mBubbles.size()); ++i)
    {
        const Bubble& b = mBubbles[i];

        // Per-bubble sine wobble perpendicular to its heading (Draw_0.gml:27-29).
        const float wave = Wave(-1.0f, 1.0f, 2.0f, count > 0.0f ? i / count : 0.0f, mTime) * 4.0f;
        float px = b.x + LengthDirX(wave, b.dir + 90.0f);
        float py = b.y + LengthDirY(wave, b.dir + 90.0f);
        ApplyCameraOffset(px, py);

        Color tint = MergeColor(baseTint, fuchsia, b.purple);
        tint.a = b.alpha;

        render2D.DrawSprite(mFrames[b.index], px, py, kBubbleOriginX, kBubbleOriginY, 1.0f, 1.0f, 0.0f, tint);
    }
}

void BackgroundComponent::Deserialize(const rapidjson::Value& value)
{
    Render2DComponent::Deserialize(value);
    SaveUtil::ReadInt("BubbleCount", mBubbleCount, value);
}
} // namespace Engine::CriticalCore
