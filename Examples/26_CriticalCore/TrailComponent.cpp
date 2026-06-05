#include "TrailComponent.h"

#include "GmHelpers.h"
#include "Render2D.h"

using namespace Engine;

namespace Engine::CriticalCore
{
void TrailComponent::Initialize()
{
    // Base caches the const transform + registers with the render service.
    Render2DComponent::Initialize();

    // We also need a mutable transform to apply the GameMaker speed/direction
    // drift each step.
    mMovableTransform = GetOwner().GetComponent<TransformComponent>();

    // oPlayerTrail/Create_0.gml: spd = random_range(0.02, 0.04) unless a spawner
    // already supplied one via SetFadeSpeed/Deserialize (sentinel < 0 => unset).
    if (mFadeSpeed < 0.0f)
    {
        mFadeSpeed = RandomRange(0.02f, 0.04f);
    }
}

void TrailComponent::Update(float deltaTime)
{
    (void)deltaTime; // fixed-step logic: one Update == one GameMaker step.
    if (mDestroyed)
    {
        return;
    }

    // GameMaker built-in motion: position += lengthdir(speed, direction).
    if (mMovableTransform != nullptr && mDriftSpeed != 0.0f)
    {
        mMovableTransform->position.x += LengthDirX(mDriftSpeed, mDirection);
        mMovableTransform->position.y += LengthDirY(mDriftSpeed, mDirection);
    }

    // oPlayerTrail/Step_0.gml: percent -= spd; destroy when fully faded.
    mPercent -= mFadeSpeed;
    if (mPercent <= 0.0f)
    {
        mDestroyed = true;
        GetOwner().GetWorld().DestroyGameObject(GetOwner().GetHandle());
    }
}

void TrailComponent::Draw(Render2D& render2D)
{
    if (mDestroyed)
    {
        return;
    }

    float x = 0.0f;
    float y = 0.0f;
    GetDrawPosition(x, y);

    const float r = mRadius * mPercent;
    if (r <= 0.0f)
    {
        return;
    }

    // oPlayerTrail/Draw_0.gml: c_ltgrey => 1px ring; otherwise a filled disc.
    if (mOutline)
    {
        render2D.DrawCircleOutline(x, y, r, mColor);
    }
    else
    {
        render2D.DrawCircleFilled(x, y, r, mColor, /*outline=*/false);
    }
}

void TrailComponent::Deserialize(const rapidjson::Value& value)
{
    Render2DComponent::Deserialize(value);

    SaveUtil::ReadFloat("Radius", mRadius, value);
    SaveUtil::ReadFloat("FadeSpeed", mFadeSpeed, value);
    SaveUtil::ReadFloat("DriftSpeed", mDriftSpeed, value);
    SaveUtil::ReadFloat("Direction", mDirection, value);
    SaveUtil::ReadBool("Outline", mOutline, value);
    SaveUtil::ReadColor("Color", mColor, value);
}
} // namespace Engine::CriticalCore
