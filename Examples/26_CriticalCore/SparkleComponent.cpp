#include "SparkleComponent.h"

#include "CriticalCore2DRenderService.h"
#include "GmHelpers.h"
#include "Render2D.h"

#include <cmath>

using namespace Engine;

namespace Engine::CriticalCore
{
void SparkleComponent::Initialize()
{
    // Base caches transform + registers with the render service.
    Render2DComponent::Initialize();

    // oSparkle/Create_0.gml:
    //   sprite_index = choose(sSparkle, sSparkle2);
    //   image_angle  = random(360);
    //   image_index  = irandom(1);            // start on frame 0 or 1
    //   image_speed  = random_range(0.95, 1.05);
    const int spriteSet = IRandom(1); // 0 => sSparkle, 1 => sSparkle2
    mAngle = RandomRange(0.0f, 360.0f);
    mFrame = static_cast<float>(IRandom(1));
    mFrameSpeed = RandomRange(0.95f, 1.05f);

    CriticalCore2DRenderService* service = GetRenderService();
    if (service != nullptr)
    {
        Render2D& render2D = service->GetRender2D();
        const std::string base = (spriteSet == 0) ? "CriticalCore/sSparkle_" : "CriticalCore/sSparkle2_";
        for (int i = 0; i < kFrameCount; ++i)
        {
            mFrames[i] = render2D.LoadTexture(base + std::to_string(i) + ".png");
        }
        mLoaded = true;
    }
}

void SparkleComponent::Update(float deltaTime)
{
    (void)deltaTime; // fixed-step: one Update == one GameMaker step.
    if (mDestroyed)
    {
        return;
    }

    // Advance the animation. GameMaker fires "Animation End" (oSparkle/Other_7.gml
    // => instance_destroy) once the index passes the last frame.
    mFrame += mFrameSpeed;
    if (static_cast<int>(std::floor(mFrame)) >= kFrameCount)
    {
        mDestroyed = true;
        GetOwner().GetWorld().DestroyGameObject(GetOwner().GetHandle());
    }
}

void SparkleComponent::Draw(Render2D& render2D)
{
    if (mDestroyed || !mLoaded)
    {
        return;
    }

    int frameIdx = static_cast<int>(std::floor(mFrame));
    if (frameIdx < 0)
    {
        frameIdx = 0;
    }
    if (frameIdx >= kFrameCount)
    {
        frameIdx = kFrameCount - 1;
    }

    float x = 0.0f;
    float y = 0.0f;
    GetDrawPosition(x, y);

    // sSparkle origin (0,0); draw_sprite_ext with image_angle, scale 1, white.
    render2D.DrawSprite(mFrames[frameIdx], x, y, 0.0f, 0.0f, 1.0f, 1.0f, mAngle, Graphics::Colors::White);
}
} // namespace Engine::CriticalCore
