#include "ScoreComponent.h"

#include "Render2D.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace Engine::CriticalCore
{
namespace
{
// #61FFA0 double-points green (oScore Draw_64.gml:4) = (97,255,160)/255.
constexpr Graphics::Color kDoubleColor{0.380392163f, 1.0f, 0.627451003f, 1.0f};

// fScore atlas line height (task 4: fScore lineHeight == 14). valign bottom =>
// the caller pre-subtracts the line height (Render2D does horizontal align only).
constexpr float kScoreLineHeight = 14.0f;

// oScore Draw_64.gml:11 — the text climbs 16px as alpha goes 1 -> 0.
constexpr float kAlphaRise = 16.0f;

// Live oScore instances — the `with(oScore)` iteration source for the Step
// anti-overlap stacking (oScore Step_0.gml:6-12). Registered in Initialize,
// removed in Terminate; never outlives the components it points to.
std::vector<ScoreComponent*>& LiveScores()
{
    static std::vector<ScoreComponent*> scores;
    return scores;
}
} // namespace

void ScoreComponent::Initialize()
{
    Render2DComponent::Initialize();
    LiveScores().push_back(this);
}

void ScoreComponent::Terminate()
{
    auto& live = LiveScores();
    live.erase(std::remove(live.begin(), live.end(), this), live.end());
    Render2DComponent::Terminate();
}

void ScoreComponent::Update(float deltaTime)
{
    (void)deltaTime; // fixed step — fade rate is per-step, not per-second

    // oScore Step_0.gml:3-4 — fade (double-points fade half as fast), self-destruct.
    mAlpha -= 0.02f - (mDouble ? 0.01f : 0.0f);
    if (mAlpha <= 0.0f)
    {
        mAlpha = 0.0f;
        // Deferred destroy (ProcessDestroyList) — safe to call mid-Update.
        GetOwner().GetWorld().DestroyGameObject(GetOwner().GetHandle());
        return;
    }

    // oScore Step_0.gml:6-12 — this (higher-alpha) popup nudges nearby
    // lower-alpha popups up 1px so stacked scores don't overlap. In the GML the
    // running instance is `other`; the iterated instance (`self`) is the one
    // pushed. Mirror that: `this` == other (caller), `score` == self (pushed).
    for (ScoreComponent* score : LiveScores())
    {
        if (score == this)
        {
            continue;
        }
        if (std::fabs(EffectiveX() - score->EffectiveX()) < 20.0f && mAlpha > score->mAlpha &&
            std::fabs(score->EffectiveY() - EffectiveY()) < 8.0f)
        {
            score->mRiseOffset += 1.0f;
        }
    }
}

void ScoreComponent::Draw(Render2D& render2D)
{
    // World-space anchor folded through the camera offset (the popup is pinned
    // in the room; oScore Draw_64.gml:11 does the same via camera_get_view_*).
    float x = GetWorldX();
    float y = GetWorldY();
    ApplyCameraOffset(x, y);

    // Rise with fade (oScore Draw_64.gml:11) + the Step stacking offset, then
    // pre-subtract the line height for the valign==bottom convention.
    const float drawY = y - kAlphaRise * (1.0f - mAlpha) - mRiseOffset - kScoreLineHeight;

    // oScore Draw_64.gml:4 — white / red(negative) / #61FFA0(double 3x).
    Graphics::Color color =
        mNegative ? Graphics::Colors::Red : (mDouble ? kDoubleColor : Graphics::Colors::White);
    color.a = mAlpha; // oScore Draw_64.gml:10 — draw_set_alpha(image_alpha)

    // oScore Draw_64.gml:8 — "-{amount}" / "{amount}x3" / "{amount}".
    std::string text;
    if (mNegative)
    {
        text = "-" + std::to_string(mAmount);
    }
    else if (mDouble)
    {
        text = std::to_string(mAmount) + "x3";
    }
    else
    {
        text = std::to_string(mAmount);
    }

    // fScore, halign center (valign bottom handled by drawY above).
    render2D.DrawText(Font2D::Score, text, x, drawY, color, TextAlign::Center);
}

void ScoreComponent::Deserialize(const rapidjson::Value& value)
{
    Render2DComponent::Deserialize(value); // "Depth"

    if (value.HasMember("amount"))
    {
        mAmount = value["amount"].GetInt();
    }
    if (value.HasMember("negative"))
    {
        mNegative = value["negative"].GetBool();
    }
    if (value.HasMember("double"))
    {
        mDouble = value["double"].GetBool();
    }
}

float ScoreComponent::EffectiveX() const
{
    return GetWorldX();
}

float ScoreComponent::EffectiveY() const
{
    return GetWorldY() - mRiseOffset - kAlphaRise * (1.0f - mAlpha);
}
} // namespace Engine::CriticalCore
