#pragma once

#include "Render2DComponent.h"

namespace Engine::CriticalCore
{
// ---------------------------------------------------------------------------
// ScoreComponent - the floating "+/- amount" score popup (port of objects/oScore).
//
// A short-lived text label that drifts up while fading out. Spawned at RUNTIME
// (NOT placed by a level) via GameWorld::CreateGameObject("...", score.json) by
// the scoring sources (tasks 23/24): they create the object, set its transform
// to the world spawn point, then push the amount/negative/double flags through
// the setters below. It self-destructs once fully faded.
//
// Source mapping (objects/oScore):
//   * Create_0.gml      : amount/double/negative defaults (here mAmount/mDouble/mNegative).
//   * Step_0.gml:3-4    : image_alpha -= 0.02 - 0.01*double; destroy at <= 0 (Update).
//   * Step_0.gml:6-12   : `with(oScore)` anti-overlap stacking — a higher-alpha
//                         popup nudges nearby lower-alpha popups up 1px/step.
//   * Draw_64.gml:3-12  : fScore, halign center / valign bottom, color
//                         white / red(negative) / #61FFA0(double 3x), text
//                         "-{amount}" / "{amount}x3" / "{amount}", drawn at
//                         y - 16*(1-alpha) so it rises as it fades.
// ---------------------------------------------------------------------------

class ScoreComponent final : public Render2DComponent
{
  public:
    SET_TYPE_ID(CustomComponentId::ScoreComponent);

    void Initialize() override;
    void Terminate() override;
    void Update(float deltaTime) override; // one call == one fixed step (GameClock::kStep)
    void Draw(Render2D& render2D) override;

    void Deserialize(const rapidjson::Value& value) override;

    // Spawn parameters — set by the scoring sources (tasks 23/24) right after
    // CreateGameObject + positioning the transform. Defaults match oScore/Create_0.
    void SetAmount(int amount)
    {
        mAmount = amount;
    }
    void SetNegative(bool negative)
    {
        mNegative = negative;
    }
    void SetDouble(bool isDouble)
    {
        mDouble = isDouble;
    }

    int GetAmount() const
    {
        return mAmount;
    }
    bool IsNegative() const
    {
        return mNegative;
    }
    bool IsDouble() const
    {
        return mDouble;
    }
    float GetAlpha() const
    {
        return mAlpha;
    }

  private:
    // World-space anchor used by the Step stacking test (oScore Draw_64.gml:9's
    // `y - 16*(1-alpha)`, including the accumulated stacking rise). Camera offset
    // is intentionally NOT folded in here — all popups share it, so it cancels in
    // the pairwise comparison.
    float EffectiveX() const;
    float EffectiveY() const;

    int mAmount = 0;        // oScore Create_0.gml:3
    bool mDouble = false;   // oScore Create_0.gml:4 (double-points 3x)
    bool mNegative = false; // oScore Create_0.gml:5 (point loss)

    float mAlpha = 1.0f;      // image_alpha (fades to 0, then self-destruct)
    float mRiseOffset = 0.0f; // accumulated `y -= 1` stacking rise (Step_0.gml:10)
};
} // namespace Engine::CriticalCore
