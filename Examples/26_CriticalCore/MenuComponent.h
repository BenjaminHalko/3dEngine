#pragma once

#include "Leaderboard.h"
#include "LeaderboardFetcher.h"
#include "Render2DComponent.h"

#include <array>
#include <string>
#include <utility>
#include <vector>

namespace Engine::CriticalCore
{
class Render2D;

// ---------------------------------------------------------------------------
// MenuComponent - the title screen / main menu (port of GameMaker oMenu).
//
// Faithful port of objects/oMenu (Create_0 / Step_0 / Draw_0 / Draw_64 /
// Alarm_0) with EVERY online branch dropped (gxGames / noInternet / OPERA /
// Firebase leaderboard API). What remains is the local, offline title screen:
//
//   * Title sprite (sTitle) drawn screen-fixed near the top third.
//   * A four-entry option list navigated with the keyboard (UP/DOWN, wrap):
//       0 = START        -> raises the "start" signal the game flow polls
//       1 = LEADERBOARD  -> opens an in-place leaderboard screen (local board)
//       2 = USERNAME     -> inline text entry (<=10 chars)
//       3 = VOLUME       -> LEFT/RIGHT slider, 0..1
//   * A render/effects toggle (CONTROL key combo, per oGlobalController) — the
//     global.render setting; persisted. It is NOT a menu list item.
//
// All settings (username / volume / render) persist through the local
// Leaderboard JSON save file (task 30, criticalcore_save.json), which replaced
// the cut online board + GameMaker ini. They are loaded on Initialize and saved
// on every change, mirroring oGlobalController/Create_0.gml:18-30.
//
// CROSS-COMPONENT SIGNALS (static, single-menu game):
//   * IsMenuActive()       — true while the menu owns the screen; flips false
//                            the moment START fires. The WallComponent (task 25)
//                            reads this to drive its outward "menu-scale" walls,
//                            and any other system that needs a menu/in-game gate.
//   * ConsumeStartRequest()— one-shot: true exactly once after START is
//                            confirmed. The game flow (task 27) polls this each
//                            fixed step and, when set, performs GameStart:
//                            play the music (MusicController), play snStart, and
//                            transition out of the menu (destroying this object).
//   The MenuComponent itself only raises the flag; it never plays the music,
//   the snStart SFX, or runs the transition — that wiring belongs to the flow.
//
// Rendering: derives from Render2DComponent. The menu is a SCREEN-FIXED UI
// overlay, so it draws at absolute 256x224 coordinates WITHOUT the camera
// offset (the GameMaker source cancels the camera by adding camera_get_view_x;
// drawing at raw RT coordinates is the equivalent in our pipeline). Depth is set
// low (drawn last / on top) so the menu sits above the background.
// ---------------------------------------------------------------------------

class MenuComponent final : public Render2DComponent
{
  public:
    SET_TYPE_ID(CustomComponentId::MenuComponent);

    void Initialize() override;
    void Terminate() override;
    void Update(float deltaTime) override; // one call == one fixed step (kStep)
    void Draw(Render2D& render2D) override;
    void DebugUI() override;

    void Deserialize(const rapidjson::Value& value) override;

    // --- Cross-component signals (read by the flow task 27 + walls task 25) ---

    // True while the title/menu is up; flips to false the instant START fires.
    static bool IsMenuActive();

    // Peek the pending start request without clearing it.
    static bool IsStartPending();

    // One-shot: returns true exactly once after START is confirmed, then clears.
    // The game flow (task 27) polls this each fixed step to launch GameStart.
    static bool ConsumeStartRequest();

    // Flow-authoritative in-game gate (pushed every fixed step by GameFlow). The
    // menu only owns the screen while NOT in-game; the instant a game starts this
    // forces the title/menu to stop drawing and consuming input, and clearing it
    // (back to title or the post-game leaderboard) restores it.
    static void SetInGame(bool inGame);

    // Game-over hand-off (GameStart.gml GameEnd -> GotoLeaderboard): the flow calls
    // this after posting the score; the dormant menu reactivates and shows the
    // leaderboard. ENTER there starts a NEW game (oLeaderboardAPI inGame branch).
    static void RequestGameOverLeaderboard();

  private:
    // Loads the title / slider sprites + bitmap fonts into the shared Render2D
    // once (lazy, on first Draw — the service-owned Render2D is guaranteed
    // initialized by then; idempotent if another component also loads them).
    void EnsureAssets(Render2D& render2D);

    // --- Per-fixed-step key edge detection (the input-fix core) ---
    //
    // The engine's InputSystem::IsKeyPressed() is a RENDER-FRAME edge: it is true
    // for EXACTLY ONE render frame (the frame after a key transitions down). The
    // menu, however, only ticks inside GameState's fixed-step subloop, which runs
    // ZERO fixed steps on most render frames (uncapped FPS => dt < 1/60, so the
    // 60Hz accumulator yields a step on only ~1 of every N render frames). The
    // single render frame carrying the IsKeyPressed edge almost never coincides
    // with a step frame, so the press is consumed by InputSystem::Update() and
    // lost before the menu ever runs => "input does not work on the title".
    //
    // Fix: edge-detect against the LEVEL key state (IsKeyDown, which stays true
    // for the WHOLE press ~ several fixed steps) sampled at the MENU's own update
    // rate. EdgePressed() reports a key that is down now but was not down at the
    // previous menu tick; SnapshotKeys() refreshes that previous-tick snapshot at
    // the end of every Update (via an RAII guard, so every early-return path keeps
    // it current). A human keypress spans multiple fixed steps, so it is caught
    // exactly once, regardless of render framerate.
    bool EdgePressed(Engine::Input::KeyCode key) const;
    void SnapshotKeys();

    // Per-fixed-step input handlers (Step_0.gml).
    void UpdateNavigation();   // option wrap + select dispatch
    void UpdateUsername();     // inline text entry while option == 2
    void UpdateVolume();       // LEFT/RIGHT slider while option == 3
    void TrimTrailingSpaces(); // mirror Step_0.gml:13-17

    // Draw helpers (Draw_0 + Draw_64).
    void DrawTitle(Render2D& render2D) const;
    void DrawMenu(Render2D& render2D);
    void DrawLeaderboardScreen(Render2D& render2D) const;

    // --- Menu state (oMenu instance vars) ---
    int mOption = 0;                 // 0..3, wraps inclusively (4 options)
    bool mShowLeaderboard = false;   // leaderboard sub-screen active
    bool mGameOverMode = false;      // leaderboard shown after GameEnd (ENTER -> new game)
    bool mSelectDisabled = false;    // skip one ENTER frame (oLeaderboardAPI disableSelect)
    float mUsernameFlash = 0.0f;     // red flash when START pressed with no name
    bool mBlink = false;             // username caret blink (Alarm_0)
    int mBlinkTimer = 0;             // frames until next blink toggle (alarm[0])

    // --- Persisted settings (Leaderboard-backed) ---
    Leaderboard mLeaderboard;        // local JSON save (settings + board)
    AsyncLeaderboardFetcher mFetcher; // off-thread Firebase fetch (poll/consume)
    std::string mUsername;           // live edit buffer (<=10 chars)
    float mVolume = 0.7f;            // 0..1 master gain

    // --- Leaderboard view (decoupled from local top-10) ---
    // mDisplayEntries holds the FULL union of local rows + raw remote rows,
    // deduped by name (best score wins), sorted DESC by score. The Leaderboard
    // class still caps its own mScores at top-10 for the save file - this is the
    // scroll-friendly browse buffer, rebuilt on Load + every fetch landing.
    std::vector<Leaderboard::Entry> mDisplayEntries;
    // Scroll state ported 1:1 from oLeaderboardAPI/Create_0.gml + Step_0.gml.
    // mScoreOffset (float, smoothed) Approach()es mScoreOffsetTarget (int, the
    // discrete "where I want to be"); mScrollSpd accelerates while UP/DOWN is
    // held (+0.05 per step, resets to 1 on release) and scales both the target
    // step AND the Approach speed; mLeaderboardMoved suppresses recentering on
    // fetch-land once the user has manually scrolled.
    float mScoreOffset = 0.0f;
    int mScoreOffsetTarget = 0;
    float mScrollSpd = 1.0f;
    bool mLeaderboardMoved = false;
    void RebuildDisplayEntries(const std::vector<RemoteEntry>& remote);
    void PositionLeaderboard();      // GameStart.gml:95 - center on mUsername (or 0)

    // --- Loaded assets (lazy) ---
    bool mAssetsLoaded = false;
    Graphics::TextureId mTitleTex = 0;
    Graphics::TextureId mAudioLineBgTex = 0;   // sAudioLine frame 0 (track)
    Graphics::TextureId mAudioLineFillTex = 0; // sAudioLine frame 1 (knob)
    Graphics::TextureId mAudioIconTex = 0;     // sAudio

    // Previous-tick key-down snapshot for EdgePressed()/SnapshotKeys() (indexed by
    // KeyCode; 512 matches InputSystem's internal key array). Refreshed at the end
    // of every Update so edges are computed at the menu's fixed-step rate.
    std::array<bool, 512> mPrevKeyDown{};

    // --- Shared signals (single-menu game) ---
    static bool sMenuActive;
    static bool sStartRequested;
    static bool sGameOverLeaderboardRequest;
    static bool sInGame;
};
} // namespace Engine::CriticalCore
