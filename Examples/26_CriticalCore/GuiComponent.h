#pragma once

#include "Render2DComponent.h"

namespace Engine::CriticalCore
{
// ---------------------------------------------------------------------------
// GuiState - the read-only HUD snapshot (mirror of oGUI's global.* fields).
//
// WIRING (documented for tasks 27 + 34):
//   * Task 27 (game flow) OWNS one GuiState and mutates it each fixed step
//     (score/round/lives/pb/flags). PB may be sourced from Leaderboard::GetPB().
//   * Task 34 hands the GuiComponent a pointer to that state via SetState(...).
//   * GuiComponent only READS it (in Draw). When the pointer is null, the
//     component falls back to its own DefaultState() so the HUD still renders
//     standalone (e.g. before the flow controller is wired).
//
// Source fields (oGUI/Create_0.gml + oGUI/Draw_64.gml):
//   score/lives/round/pb -> global.score/lives/round/pb
//   newPB                -> NEW PB! indicator (Draw_64:14-18)
//   nextRound            -> ROUND COMPLETE! banner (Draw_64:38-49)
//   roundIntro           -> ROUND N intro (Draw_64:51-55)
//   inGame               -> gates MOVE TO START (Draw_64:63)
//   gameOver             -> GAME OVER (Draw_64:57-61)
//   displayExtraLives    -> +1 LIFE vs +12500 POINTS (Draw_64:43-48)
// ---------------------------------------------------------------------------
struct GuiState
{
    int score = 0;
    int lives = 3;
    int round = 1;
    int pb = 0;
    bool newPB = false;
    bool nextRound = false;
    bool roundIntro = false;
    bool inGame = false;
    bool gameOver = false;
    bool displayExtraLives = true;
};

// ---------------------------------------------------------------------------
// GuiComponent - the heads-up display (port of objects/oGUI).
//
// Drawn entirely in 256x224 SCREEN space (fixed corners + centered banners), so
// it does NOT fold in the camera offset. Placed by the level (task 33) via the
// gui.json template; it reads the shared GuiState (above) for all dynamic text.
//
// All text uses Render2D Font2D::Font (the fFont atlas: uppercase + digits), the
// only font oGUI/Draw_64 ever sets. Score popups (oScore) use Font2D::Score —
// that is a different component (ScoreComponent).
//
// The two oGUI alarms are reproduced here as frame counters on the fixed clock
// (one Update() == one fixed step):
//   * Alarm_0 (Draw_64:32-36 READY? blink): gameStart climbs 1->11 (+1 every 5
//     steps); "READY?" shows on odd values; on the 12th step it completes
//     (== GML GameStart()) and latches ConsumeReadyComplete() for the flow.
//   * Alarm_1 (Draw_64:63-67 MOVE TO START): after a delay it raises the tutorial
//     flag; the flow clears it (SetMoveTutorial(false)) once the player moves.
// ---------------------------------------------------------------------------
class GuiComponent final : public Render2DComponent
{
  public:
    SET_TYPE_ID(CustomComponentId::GuiComponent);

    void Initialize() override;
    void Terminate() override;
    void Update(float deltaTime) override; // one call == one fixed step
    void Draw(Render2D& render2D) override;

    void Deserialize(const rapidjson::Value& value) override;

    // --- Shared HUD state (task 34 wires the pointer; task 27 owns/mutates it) ---
    void SetState(const GuiState* state)
    {
        mState = state;
    }
    // Standalone fallback state (used when no pointer is wired).
    GuiState& DefaultState()
    {
        return mDefaultState;
    }

    // --- oGUI Alarm_0: READY? countdown blink ---
    void StartReadyCountdown();
    // True exactly once after the 12-step countdown completes (== GameStart());
    // reading it clears the latch.
    bool ConsumeReadyComplete();

    // --- oGUI Alarm_1: MOVE TO START tutorial ---
    void ArmMoveTutorial(int delayFrames);
    void SetMoveTutorial(bool show)
    {
        mMoveTutorial = show;
    }

  private:
    const GuiState& State() const
    {
        return (mState != nullptr) ? *mState : mDefaultState;
    }

    const GuiState* mState = nullptr;
    GuiState mDefaultState;

    // oGUI Alarm_0 (gameStart blink) frame counter.
    bool mReadyActive = false;
    int mReadyFrameTimer = 0;
    int mGameStart = 0;
    bool mReadyComplete = false;

    // oGUI Alarm_1 (tutorial delay) frame counter.
    bool mTutorialArmed = false;
    int mTutorialFrameTimer = 0;
    bool mMoveTutorial = false;
};
} // namespace Engine::CriticalCore
