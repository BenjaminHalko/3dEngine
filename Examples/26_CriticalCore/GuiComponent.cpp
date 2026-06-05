#include "GuiComponent.h"

#include "Render2D.h"

#include <cstdio>
#include <string>

namespace Engine::CriticalCore
{
namespace
{
// #61FFA0 highlight green (oGUI Draw_64.gml:15,41 — NEW PB! + ROUND COMPLETE!).
constexpr Graphics::Color kHighlightColor{0.380392163f, 1.0f, 0.627451003f, 1.0f};

// fFont atlas line height (task 4: fFont lineHeight == 8). Two-line HUD labels
// step down by this; valign==middle banners pre-subtract half of it.
constexpr float kFontLineHeight = 8.0f;
constexpr float kFontHalfLine = kFontLineHeight * 0.5f;

// oGUI Draw_64.gml:8-10 — string_format(num,5,0) space-padded then spaces->"0",
// i.e. a 5-wide zero-padded integer.
std::string DisplayNumber(int value)
{
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%05d", value);
    return std::string(buffer);
}

const GuiState* gActiveState = nullptr;
} // namespace

void GuiComponent::SetActiveState(const GuiState* state)
{
    gActiveState = state;
}

const GuiState* GuiComponent::GetActiveState()
{
    return gActiveState;
}

void GuiComponent::Initialize()
{
    Render2DComponent::Initialize();

    if (mState == nullptr)
    {
        mState = gActiveState;
    }
}

void GuiComponent::Terminate()
{
    Render2DComponent::Terminate();
}

void GuiComponent::Update(float deltaTime)
{
    (void)deltaTime; // fixed step — alarms count fixed steps, not seconds

    // oGUI Alarm_0 — gameStart +1 every 5 steps; the 12th step == GameStart().
    if (mReadyActive)
    {
        --mReadyFrameTimer;
        if (mReadyFrameTimer <= 0)
        {
            ++mGameStart;
            if (mGameStart == 12)
            {
                mGameStart = 0;
                mReadyActive = false;
                mReadyComplete = true; // latched for ConsumeReadyComplete()
            }
            else
            {
                mReadyFrameTimer = 5; // GML alarm[0] = 5
            }
        }
    }

    // oGUI Alarm_1 — after the delay, raise the tutorial flag. The flow clears it
    // (SetMoveTutorial(false)) once the player has moved (GML's playerHasMoved).
    if (mTutorialArmed)
    {
        --mTutorialFrameTimer;
        if (mTutorialFrameTimer <= 0)
        {
            mMoveTutorial = true;
            mTutorialArmed = false;
        }
    }
}

void GuiComponent::Draw(Render2D& render2D)
{
    const GuiState& s = State();
    const Graphics::Color white = Graphics::Colors::White;

    constexpr float kCenterX = static_cast<float>(kInternalWidth) * 0.5f;  // 128
    constexpr float kCenterY = static_cast<float>(kInternalHeight) * 0.5f; // 112

    // --- TL: SCORE (Draw_64:13). Second line is indented one space then 5-pad. ---
    render2D.DrawText(Font2D::Font, "SCORE", 8.0f, 8.0f, white);
    render2D.DrawText(Font2D::Font, " " + DisplayNumber(s.score), 8.0f, 8.0f + kFontLineHeight,
                      white);

    // --- NEW PB! indicator (Draw_64:14-18, #61FFA0). ---
    if (s.newPB)
    {
        render2D.DrawText(Font2D::Font, "NEW PB!", 8.0f, 32.0f, kHighlightColor);
    }

    // --- BL: PB (Draw_64:25). ---
    render2D.DrawText(Font2D::Font, "PB", 8.0f, 200.0f, white);
    render2D.DrawText(Font2D::Font, " " + DisplayNumber(s.pb), 8.0f, 200.0f + kFontLineHeight,
                      white);

    // --- TR: ROUND (Draw_64:29) ; BR: LIVES (Draw_64:30). Left-aligned at x=200. ---
    render2D.DrawText(Font2D::Font, "ROUND", 200.0f, 8.0f, white);
    render2D.DrawText(Font2D::Font, " " + std::to_string(s.round), 200.0f, 8.0f + kFontLineHeight,
                      white);
    render2D.DrawText(Font2D::Font, "LIVES", 200.0f, 200.0f, white);
    render2D.DrawText(Font2D::Font, " " + std::to_string(s.lives), 200.0f,
                      200.0f + kFontLineHeight, white);

    // --- READY? blink (Draw_64:32-36, Alarm_0; shown on odd gameStart). ---
    if (mGameStart % 2 == 1)
    {
        render2D.DrawText(Font2D::Font, "READY?", kCenterX, kCenterY - kFontHalfLine, white,
                          TextAlign::Center);
    }

    // --- Round-complete banner (Draw_64:38-49, #61FFA0). ---
    if (s.nextRound)
    {
        render2D.DrawText(Font2D::Font, "ROUND COMPLETE!", kCenterX,
                          (kCenterY - 40.0f) - kFontHalfLine, kHighlightColor, TextAlign::Center);
        if (s.displayExtraLives)
        {
            render2D.DrawText(Font2D::Font, "+10000 POINTS", kCenterX,
                              (kCenterY + 40.0f) - kFontHalfLine, kHighlightColor,
                              TextAlign::Center);
            render2D.DrawText(Font2D::Font, "+1 LIFE", kCenterX,
                              (kCenterY + 50.0f) - kFontHalfLine, kHighlightColor,
                              TextAlign::Center);
        }
        else
        {
            render2D.DrawText(Font2D::Font, "+12500 POINTS", kCenterX,
                              (kCenterY + 40.0f) - kFontHalfLine, kHighlightColor,
                              TextAlign::Center);
        }
    }

    // --- ROUND N intro (Draw_64:51-55). ---
    if (s.roundIntro)
    {
        render2D.DrawText(Font2D::Font, "ROUND " + std::to_string(s.round), kCenterX,
                          kCenterY - kFontHalfLine, white, TextAlign::Center);
    }

    // --- GAME OVER (Draw_64:57-61). The cut online oLeaderboardAPI.draw gate is
    //     dropped (out of scope); the local Leaderboard replaces it. ---
    if (s.gameOver)
    {
        render2D.DrawText(Font2D::Font, "GAME OVER", kCenterX, 46.0f - kFontHalfLine, white,
                          TextAlign::Center);
    }

    // --- MOVE TO START tutorial (Draw_64:63-67, Alarm_1; gated on inGame). ---
    if (mMoveTutorial && s.inGame)
    {
        render2D.DrawText(Font2D::Font, "MOVE TO START", kCenterX, kCenterY - kFontHalfLine, white,
                          TextAlign::Center);
    }
}

void GuiComponent::Deserialize(const rapidjson::Value& value)
{
    Render2DComponent::Deserialize(value); // "Depth"
}

void GuiComponent::StartReadyCountdown()
{
    mGameStart = 0;
    mReadyFrameTimer = 5; // GML arms alarm[0] = 5
    mReadyActive = true;
    mReadyComplete = false;
}

bool GuiComponent::ConsumeReadyComplete()
{
    const bool done = mReadyComplete;
    mReadyComplete = false;
    return done;
}

void GuiComponent::ArmMoveTutorial(int delayFrames)
{
    mTutorialFrameTimer = delayFrames;
    mTutorialArmed = true;
    mMoveTutorial = false;
}
} // namespace Engine::CriticalCore
