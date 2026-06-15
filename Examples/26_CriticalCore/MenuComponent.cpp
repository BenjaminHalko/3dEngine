#include "MenuComponent.h"

#include "GmHelpers.h"
#include "LeaderboardFetcher.h"
#include "Render2D.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>

using namespace Engine;
using Engine::Input::InputSystem;
using Engine::Input::KeyCode;

namespace Engine::CriticalCore
{
namespace
{
// Source-space layout constants (Draw_0 / Draw_64). RES_WIDTH=256, RES_HEIGHT=224.
constexpr float kTitleX = kInternalWidth * 0.5f + 3.0f;      // RES_WIDTH/2 + 3  = 131
constexpr float kTitleY = kInternalHeight / 3.0f + 4.0f;     // RES_HEIGHT/3 + 4 ~= 78.67
constexpr float kTitleOriginX = 70.0f;                       // sTitle origin
constexpr float kTitleOriginY = 26.0f;

constexpr float kMenuX = kInternalWidth * 0.5f - 34.0f;      // RES_WIDTH/2 - 34 = 94
constexpr float kMenuY = kInternalHeight * 0.5f + 8.0f;      // RES_HEIGHT/2 + 8 = 120
constexpr float kLineStep = 16.0f;                           // vertical option spacing

constexpr int kOptionCount = 4;                              // START / LEADERBOARD / USERNAME / VOLUME
constexpr int kOptStart = 0;
constexpr int kOptLeaderboard = 1;
constexpr int kOptUsername = 2;
constexpr int kOptVolume = 3;

constexpr int kMaxUsername = 10;                             // Step_0.gml:57 (<=10)
constexpr float kVolumeStep = 0.1f;                          // Step_0.gml:76
constexpr int kBlinkFrames = 30;                             // Alarm_0 reset (Step_0.gml:56)
constexpr float kSliderPixels = 60.0f;                       // sAudioLine width

// scoresPerPage from oLeaderboardAPI/Create_0.gml:9. Rows beyond this fade out
// per the alpha-by-distance formula in Draw_64.gml:28.
constexpr int kLeaderboardVisibleRows = 8;

// GameMaker named colors used by the menu.
const Graphics::Color kWhite = Graphics::Colors::White;
const Graphics::Color kRed = Graphics::Colors::Red;
const Graphics::Color kDkGray = Graphics::Color{0.25f, 0.25f, 0.25f, 1.0f};  // c_dkgray (64,64,64)
const Graphics::Color kYellow = Graphics::Color{1.0f, 1.0f, 0.0f, 1.0f};     // c_yellow (player row)

// GameMaker ordinal suffix for a 1-based rank (oLeaderboardAPI Draw_64.gml:35-38).
std::string Ordinal(int n)
{
    const int tens = n % 100;
    const int ones = n % 10;
    if (ones == 1 && tens != 11)
        return std::to_string(n) + "st";
    if (ones == 2 && tens != 12)
        return std::to_string(n) + "nd";
    if (ones == 3 && tens != 13)
        return std::to_string(n) + "rd";
    return std::to_string(n) + "th";
}

// Held = current level key state (true for the whole press). Used directly for
// modifier keys (shift) and as the basis for the menu's own edge detection.
bool Held(KeyCode key)
{
    const InputSystem* in = InputSystem::Get();
    return (in != nullptr) && in->IsKeyDown(key);
}

// snBlip - menu nav / select / volume blip (oMenu/Step_0.gml:25,46,52,79).
Engine::Audio::SoundId BlipSfx()
{
    static Engine::Audio::SoundId id =
        Engine::Audio::SoundEffectManager::Get()->Load("CriticalCore/snBlip.wav");
    return id;
}
void PlayBlip()
{
    if (Engine::Audio::SoundEffectManager* sfx = Engine::Audio::SoundEffectManager::Get())
    {
        sfx->Play(BlipSfx());
    }
}
} // namespace

// Shared single-menu signals.
bool MenuComponent::sMenuActive = false;
bool MenuComponent::sStartRequested = false;
bool MenuComponent::sGameOverLeaderboardRequest = false;
bool MenuComponent::sInGame = false;

void MenuComponent::SetInGame(bool inGame)
{
    sInGame = inGame;
}

bool MenuComponent::IsMenuActive()
{
    return sMenuActive && !sInGame;
}

bool MenuComponent::IsStartPending()
{
    return sStartRequested;
}

bool MenuComponent::ConsumeStartRequest()
{
    const bool requested = sStartRequested;
    sStartRequested = false;
    return requested;
}

void MenuComponent::RequestGameOverLeaderboard()
{
    sGameOverLeaderboardRequest = true;
}

void MenuComponent::RequestTitle()
{
    // Re-arm the resident menu to the plain title after an ESC-in-game abandon.
    // START left sMenuActive=false; the in-place title reset keeps the menu
    // object alive (no world rebuild), so we restore the screen gate here and
    // drop any leaderboard sub-screen + game-over hand-off latch.
    sMenuActive = true;
    sGameOverLeaderboardRequest = false;
}

bool MenuComponent::EdgePressed(KeyCode key) const
{
    const InputSystem* in = InputSystem::Get();
    if (in == nullptr)
    {
        return false;
    }
    const int idx = static_cast<int>(key);
    if (idx < 0 || idx >= static_cast<int>(mPrevKeyDown.size()))
    {
        return false;
    }
    // Edge vs the menu's previous fixed-step tick (NOT IsKeyPressed's render-frame
    // edge, which the sparse fixed-step loop drops): down now, up last menu tick.
    return in->IsKeyDown(key) && !mPrevKeyDown[idx];
}

void MenuComponent::SnapshotKeys()
{
    const InputSystem* in = InputSystem::Get();
    if (in == nullptr)
    {
        mPrevKeyDown.fill(false);
        return;
    }
    for (int i = 0; i < static_cast<int>(mPrevKeyDown.size()); ++i)
    {
        mPrevKeyDown[i] = in->IsKeyDown(static_cast<KeyCode>(i));
    }
}

void MenuComponent::Initialize()
{
    // Base: cache transform + register with the 2D render service.
    Render2DComponent::Initialize();

    // The menu owns the screen until START fires.
    sMenuActive = true;
    sStartRequested = false;

    mOption = 0;
    mShowLeaderboard = false;
    mUsernameFlash = 0.0f;
    mBlink = false;
    mBlinkTimer = kBlinkFrames;

    // Load persisted settings (oGlobalController/Create_0.gml:18-30), now backed
    // by the local Leaderboard JSON save (task 30).
    mLeaderboard.Load();
    mUsername = mLeaderboard.GetUsername();
    mVolume = mLeaderboard.GetVolume();

    // Create_0 guard: clamp an over-long persisted name.
    if (mUsername.size() > static_cast<std::size_t>(kMaxUsername))
    {
        mUsername.clear();
    }

    // audio_master_gain(global.audioVol).
    if (Engine::Audio::AudioSystem* audio = Engine::Audio::AudioSystem::Get())
    {
        audio->SetMasterVolume(mVolume);
    }
}

void MenuComponent::Terminate()
{
    // Leaving the menu releases the screen gate (defensive; the flow normally
    // flips this on START before destroying the object).
    sMenuActive = false;
    Render2DComponent::Terminate();
}

void MenuComponent::Deserialize(const rapidjson::Value& value)
{
    // Base reads "Depth"; the menu has no further JSON-tunable fields (settings
    // come from the save file, not the level template).
    Render2DComponent::Deserialize(value);
}

void MenuComponent::Update(float deltaTime)
{
    (void)deltaTime; // fixed-step logic; menu input is edge-driven, not dt-scaled.

    // Refresh the previous-tick key snapshot on EVERY exit path so EdgePressed()
    // stays correct no matter which branch returns (see MenuComponent.h).
    struct SnapshotGuard
    {
        MenuComponent* self;
        ~SnapshotGuard()
        {
            self->SnapshotKeys();
        }
    } snapshotGuard{this};

    // Runs even while !sMenuActive (the dormant menu owns the post-game board), so
    // it must precede the gate below. GameStart.gml GameEnd -> GotoLeaderboard.
    if (sGameOverLeaderboardRequest)
    {
        sGameOverLeaderboardRequest = false;
        sMenuActive = true;
        mShowLeaderboard = true;
        mGameOverMode = true;
        mSelectDisabled = true; // oLeaderboardAPI disableSelect (ignore stale ENTER)
        mLeaderboard.Load();
        // Refresh the in-memory username from the freshly-loaded disk state
        // BEFORE PositionLeaderboard - it matches the row GameFlow::GameEnd just
        // Posted under, so the centering finds the player on the local board.
        mUsername = mLeaderboard.GetUsername();
        mLeaderboardMoved = false; // GotoLeaderboard (GameStart.gml:111-117)
        mScrollSpd = 1.0f;
        RebuildDisplayEntries({}); // seed local-only view until the fetch lands
        PositionLeaderboard();
        mFetcher.Start(kRemoteLeaderboardUrl);
    }

    // Pull in any completed background fetch BEFORE drawing decisions this tick.
    // Runs every Update including game-running ticks so a fetch kicked off at
    // game-over lands the moment the worker finishes, with no extra polling.
    if (mFetcher.Poll() == AsyncLeaderboardFetcher::State::Ready)
    {
        std::vector<RemoteEntry> remote;
        mFetcher.Consume(remote);

        // Build the display + recenter BEFORE writing remote into the local
        // board. RebuildDisplayEntries reads mLeaderboard.Entries(), and
        // MergeRemoteEntry trims that storage to kMaxScores=10 after every
        // insert - merging 14k+ high-score remote rows first would push the
        // player's own local rows out of the top-10 and the display would
        // never see them (and PositionLeaderboard could never find mUsername).
        RebuildDisplayEntries(remote);
        // Other_70.gml:34-35 — only recenter on Firebase-Read-complete when the
        // user has NOT manually scrolled (mLeaderboardMoved == moved in GML).
        if (!mLeaderboardMoved)
        {
            PositionLeaderboard();
        }

        // After the display is locked in, fold remote into the local board so
        // the retroactive level-patch heuristic in Leaderboard::InsertOrUpdate
        // (same name + same score + local level 0 -> adopt remote level) can
        // heal pre-`level`-field legacy rows on subsequent reloads. The trim
        // pollution this causes is transient: each menu open does Load() which
        // reloads the pure local top-10 from disk.
        for (const RemoteEntry& e : remote)
        {
            mLeaderboard.MergeRemoteEntry(e.name, e.score, e.level);
        }
    }

    // Flow-authoritative gate: the menu never owns the screen while a game is
    // running (it hides + stops consuming input the instant GameStart flips
    // sInGame), regardless of how START was reached.
    if (sInGame || !sMenuActive)
    {
        return;
    }

    if (mShowLeaderboard)
    {
        if (mSelectDisabled)
        {
            mSelectDisabled = false;
        }
        else if (EdgePressed(KeyCode::RETURN))
        {
            if (mGameOverMode)
            {
                // oLeaderboardAPI Step_0:24-29 inGame branch: ENTER -> NEW game.
                mShowLeaderboard = false;
                mGameOverMode = false;
                sStartRequested = true;
                sMenuActive = false;
            }
            else
            {
                mShowLeaderboard = false;
            }
        }
        else if (EdgePressed(KeyCode::ESCAPE))
        {
            mShowLeaderboard = false;
            mGameOverMode = false;
        }
        else
        {
            // oLeaderboardAPI/Step_0.gml:9-22 - continuous-held scroll with
            // scrollSpd acceleration. Target jumps in integer steps only when
            // the smoothed offset has caught up (line 9 gate); the smoothed
            // offset Approach()es the target with speed proportional to
            // scrollSpd; scrollSpd accelerates +0.05/step while UP/DOWN is held
            // and snaps back to 1 on release.
            const int keyUp = (Held(KeyCode::UP) || Held(KeyCode::W)) ? 1 : 0;
            const int keyDown = (Held(KeyCode::DOWN) || Held(KeyCode::S)) ? 1 : 0;
            const int total = static_cast<int>(mDisplayEntries.size());
            const int maxTarget = std::max(0, total - kLeaderboardVisibleRows);

            if (std::fabs(mScoreOffset - static_cast<float>(mScoreOffsetTarget)) < 0.001f)
            {
                if (maxTarget == 0)
                {
                    mScoreOffsetTarget = 0;
                }
                else
                {
                    const int step = static_cast<int>(std::round(
                        static_cast<float>(keyDown - keyUp) * std::max(mScrollSpd - 1.0f, 1.0f)));
                    mScoreOffsetTarget = std::clamp(mScoreOffsetTarget + step, 0, maxTarget);
                }
            }

            mScoreOffset = Approach(mScoreOffset, static_cast<float>(mScoreOffsetTarget),
                                    std::max(mScrollSpd - 1.0f, 1.0f) * 0.4f);

            if (keyDown - keyUp != 0)
            {
                mScrollSpd += 0.05f;
                mLeaderboardMoved = true;
            }
            else
            {
                mScrollSpd = 1.0f;
            }
        }
        mUsernameFlash = Approach(mUsernameFlash, 0.0f, 0.04f);
        return;
    }

    UpdateNavigation();

    if (mOption == kOptUsername)
    {
        UpdateUsername();
    }
    if (mOption == kOptVolume)
    {
        UpdateVolume();
    }

    // Step_0.gml:87 — flash decays toward 0 every step.
    mUsernameFlash = Approach(mUsernameFlash, 0.0f, 0.04f);
}

void MenuComponent::UpdateNavigation()
{
    // Arrows always navigate; W/S also navigate EXCEPT while editing the username
    // (Step_0.gml:10 — on the username row, W/S type instead of moving).
    const bool allowLetters = (mOption != kOptUsername);
    const bool up = EdgePressed(KeyCode::UP) || (allowLetters && EdgePressed(KeyCode::W));
    const bool down = EdgePressed(KeyCode::DOWN) || (allowLetters && EdgePressed(KeyCode::S));

    const int delta = (down ? 1 : 0) - (up ? 1 : 0);
    if (delta != 0)
    {
        // Leaving the username row trims trailing spaces + persists (Step_0:12-21).
        if (mOption == kOptUsername)
        {
            TrimTrailingSpaces();
            mLeaderboard.SetUsername(mUsername);
            mLeaderboard.Save();
        }

        // GameMaker Wrap(option, 0, 3) is the integer-inclusive branch => 4
        // options cycling 0..3 (see learnings task-8 Wrap deviation note).
        mOption = ((mOption + delta) % kOptionCount + kOptionCount) % kOptionCount;

        PlayBlip(); // Step_0.gml:25 - nav blip on option change.
    }

    const bool select = EdgePressed(KeyCode::RETURN);

    if (mOption == kOptStart && select)
    {
        // Step_0.gml:31-48 (online branches dropped): require a username, then
        // raise the start signal the flow polls. The flow performs GameStart
        // (play music, snStart SFX, transition + destroy this object).
        if (!mUsername.empty())
        {
            // Persist the username NOW so GameFlow::GameEnd's disk reload sees
            // the freshly-typed name. Without this, pressing RETURN from the
            // START row while a name edit is still in-memory-only would post
            // the death-screen row under the previous saved name.
            TrimTrailingSpaces();
            mLeaderboard.SetUsername(mUsername);
            mLeaderboard.Save();
            sStartRequested = true;
            sMenuActive = false;
        }
        else
        {
            // No name -> red flash prompt (Step_0.gml:45-46).
            mUsernameFlash = 1.0f;
            PlayBlip(); // Step_0.gml:46 - blip on the empty-username miss.
        }
    }
    else if (mOption == kOptLeaderboard && select)
    {
        // Step_0.gml:50-53 GotoLeaderboard, now a local in-place screen.
        mLeaderboard.Load(); // refresh to show the latest board
        mLeaderboardMoved = false; // GotoLeaderboard (GameStart.gml:111-117)
        mScrollSpd = 1.0f;
        RebuildDisplayEntries({}); // seed local-only view until the fetch lands
        PositionLeaderboard();
        mFetcher.Start(kRemoteLeaderboardUrl);
        mShowLeaderboard = true;
        PlayBlip(); // Step_0.gml:52 - blip on the leaderboard select.
    }
}

void MenuComponent::TrimTrailingSpaces()
{
    while (!mUsername.empty() && mUsername.back() == ' ')
    {
        mUsername.pop_back();
    }
}

void MenuComponent::UpdateUsername()
{
    // Caret blink while editing (Alarm_0 toggles blink every 30 frames).
    if (--mBlinkTimer <= 0)
    {
        mBlink = !mBlink;
        mBlinkTimer = kBlinkFrames;
    }

    // Backspace removes the last character (Step_0.gml:57 vk_backspace branch).
    if (EdgePressed(KeyCode::BACKSPACE) && !mUsername.empty())
    {
        mUsername.pop_back();
    }

    if (mUsername.size() >= static_cast<std::size_t>(kMaxUsername))
    {
        return; // length cap (<=10)
    }

    const bool shift = Held(KeyCode::LSHIFT) || Held(KeyCode::RSHIFT);

    // Letters A..Z (case follows shift; fScore atlas is full ASCII).
    for (int c = 'A'; c <= 'Z'; ++c)
    {
        if (EdgePressed(static_cast<KeyCode>(c)))
        {
            const char ch = shift ? static_cast<char>(c)
                                  : static_cast<char>(c - 'A' + 'a');
            mUsername.push_back(ch);
        }
    }

    // Digits 0..9.
    for (int c = '0'; c <= '9'; ++c)
    {
        if (EdgePressed(static_cast<KeyCode>(c)))
        {
            mUsername.push_back(static_cast<char>(c));
        }
    }

    // Space allowed only after at least one character (Step_0.gml:57 vk_space guard).
    if (EdgePressed(KeyCode::SPACE) && !mUsername.empty())
    {
        mUsername.push_back(' ');
    }
}

void MenuComponent::UpdateVolume()
{
    // LEFT/RIGHT (and A/D) adjust the slider (Step_0.gml:72-84).
    const bool left = EdgePressed(KeyCode::LEFT) || EdgePressed(KeyCode::A);
    const bool right = EdgePressed(KeyCode::RIGHT) || EdgePressed(KeyCode::D);

    const int delta = (right ? 1 : 0) - (left ? 1 : 0);
    if (delta == 0)
    {
        return;
    }

    mVolume = std::clamp(mVolume + static_cast<float>(delta) * kVolumeStep, 0.0f, 1.0f);
    mLeaderboard.SetVolume(mVolume);
    if (Engine::Audio::AudioSystem* audio = Engine::Audio::AudioSystem::Get())
    {
        audio->SetMasterVolume(mVolume); // audio_master_gain(global.audioVol)
    }
    mLeaderboard.Save();
    PlayBlip(); // Step_0.gml:79 - blip on a volume change.
}

void MenuComponent::EnsureAssets(Render2D& render2D)
{
    if (mAssetsLoaded)
    {
        return;
    }

    mTitleTex = render2D.LoadTexture("CriticalCore/sTitle.png");
    mAudioLineBgTex = render2D.LoadTexture("CriticalCore/sAudioLine_0.png");
    mAudioLineFillTex = render2D.LoadTexture("CriticalCore/sAudioLine_1.png");
    mAudioIconTex = render2D.LoadTexture("CriticalCore/sAudio.png");

    // Bitmap fonts (idempotent across components; shared Render2D instance).
    render2D.LoadFont(Font2D::Font, "Assets/Fonts/CriticalCore/fFont.png", "Assets/Fonts/CriticalCore/fFont.json");
    render2D.LoadFont(Font2D::Score, "Assets/Fonts/CriticalCore/fScore.png", "Assets/Fonts/CriticalCore/fScore.json");

    mAssetsLoaded = true;
}

void MenuComponent::Draw(Render2D& render2D)
{
    EnsureAssets(render2D);

    // While a game runs (sInGame) or the menu has been dismissed, draw nothing -
    // the menu only paints the title and the post-game leaderboard.
    if (sInGame || !sMenuActive)
    {
        return;
    }

    if (mShowLeaderboard)
    {
        DrawLeaderboardScreen(render2D);
        return;
    }

    DrawTitle(render2D);
    DrawMenu(render2D);
}

void MenuComponent::DrawTitle(Render2D& render2D) const
{
    // Draw_0.gml:4 — sTitle at (RES_WIDTH/2+3, RES_HEIGHT/3+4). The GameMaker
    // source adds camera_get_view_x to stay screen-fixed; here we draw at raw
    // 256x224 RT coordinates (no camera offset), which is screen-fixed.
    // NOTE: optional shBlur glow (task 32) is DEFERRED — the render service
    // exposes no ping-pong glow buffers, so the title draws crisp without glow.
    render2D.DrawSprite(mTitleTex, kTitleX, kTitleY, kTitleOriginX, kTitleOriginY, 1.0f, 1.0f, 0.0f, kWhite);
}

void MenuComponent::DrawMenu(Render2D& render2D)
{
    // Draw_64.gml port (fFont, left/top aligned).
    float menuY = kMenuY;

    // Selection arrow ">" (Step_0/Draw_64 :11). The +4 nudge on the VOLUME row
    // lines the arrow up with the lower slider.
    const float arrowY = menuY + kLineStep * static_cast<float>(mOption) + (mOption == kOptVolume ? 4.0f : 0.0f);
    render2D.DrawText(Font2D::Font, ">", kMenuX - 8.0f, arrowY, kWhite);

    render2D.DrawText(Font2D::Font, "START", kMenuX, menuY, kWhite);

    menuY += kLineStep; // 136
    render2D.DrawText(Font2D::Font, "LEADERBOARD", kMenuX, menuY, kWhite);

    menuY += kLineStep; // 152 — USERNAME label with the red miss-flash + jitter.
    const Graphics::Color flashCol = MergeColor(kWhite, kRed, mUsernameFlash);
    const float jx = RandomRange(-2.0f, 2.0f) * mUsernameFlash;
    const float jy = RandomRange(-2.0f, 2.0f) * mUsernameFlash;
    render2D.DrawText(Font2D::Font, "USERNAME", kMenuX + jx, menuY + jy, flashCol);

    menuY += 5.0f; // 157 — the username value, drawn in the fScore atlas.
    std::string shown = mUsername;
    if (mBlink && mOption == kOptUsername)
    {
        shown += "_";
    }
    if (mUsername.empty())
    {
        render2D.DrawText(Font2D::Score, "ENTER USERNAME", kMenuX + 16.0f, menuY, kDkGray);
    }
    render2D.DrawText(Font2D::Score, shown, kMenuX + 16.0f, menuY, kWhite);

    menuY += 18.0f; // 175 — volume slider (sAudioLine track + knob + sAudio icon).
    render2D.DrawSprite(mAudioLineBgTex, kMenuX, menuY, 0.0f, 2.0f, 1.0f, 1.0f, 0.0f, kWhite);
    const float knobX = kMenuX + std::round(kSliderPixels * mVolume);
    // sAudioLine frame 1 is a single 1px column at the texture's left edge; the
    // sprite quad's leftmost texel does not rasterize through DrawSprite, so the
    // moving handle is drawn as the equivalent 1px x 5px vertical tick (sAudioLine
    // is 5px tall, origin y=2 => spans menuY-2 .. menuY+2).
    render2D.DrawLine(knobX, menuY - 2.0f, knobX, menuY + 3.0f, 1.0f, kWhite);
    render2D.DrawSprite(mAudioIconTex, kMenuX + 62.0f, menuY, 0.0f, 3.0f, 1.0f, 1.0f, 0.0f, kWhite);
}

void MenuComponent::RebuildDisplayEntries(const std::vector<RemoteEntry>& remote)
{
    // Union local top-10 + raw remote into a per-name best-{score,level} map,
    // then flush to a flat vector and stable-sort DESC by score. unordered_map
    // iteration order is unspecified, but the post-sort ordering is fully
    // deterministic for any given (local, remote) pair, so the rendered board
    // does not flicker. level rides with the winning score (matches
    // Leaderboard::InsertOrUpdate / oLeaderboardAPI LeaderboardPost.gml:66-67).
    struct Best
    {
        int score;
        int level;
    };
    std::unordered_map<std::string, Best> best;
    best.reserve(mLeaderboard.Entries().size() + remote.size());
    for (const Leaderboard::Entry& e : mLeaderboard.Entries())
    {
        auto [it, inserted] = best.emplace(e.name, Best{e.score, e.level});
        if (!inserted && e.score > it->second.score)
        {
            it->second = {e.score, e.level};
        }
    }
    for (const RemoteEntry& r : remote)
    {
        if (r.name.empty())
        {
            continue;
        }
        auto [it, inserted] = best.emplace(r.name, Best{r.score, r.level});
        if (!inserted && r.score > it->second.score)
        {
            it->second = {r.score, r.level};
        }
    }
    mDisplayEntries.clear();
    mDisplayEntries.reserve(best.size());
    for (const auto& [name, b] : best)
    {
        mDisplayEntries.push_back({name, b.score, b.level});
    }
    std::stable_sort(
        mDisplayEntries.begin(), mDisplayEntries.end(),
        [](const Leaderboard::Entry& a, const Leaderboard::Entry& b) { return a.score > b.score; });
}

void MenuComponent::PositionLeaderboard()
{
    // GameStart.gml:95-109 PositionLeaderboard. Center the viewport on the
    // player's row at index-5 (so they appear ~5 rows from the top of the
    // page), clamped to [0, length-perPage]. Snap both offset AND target so
    // the recenter does NOT animate (matches `scoreOffset = scoreOffsetTarget`
    // in GameStart.gml:103). If the player is absent, anchor at row 0.
    int idx = -1;
    if (!mUsername.empty())
    {
        for (int i = 0; i < static_cast<int>(mDisplayEntries.size()); ++i)
        {
            if (mDisplayEntries[i].name == mUsername)
            {
                idx = i;
                break;
            }
        }
    }
    const int total = static_cast<int>(mDisplayEntries.size());
    const int maxTarget = std::max(0, total - kLeaderboardVisibleRows);
    if (idx >= 0)
    {
        mScoreOffsetTarget = std::clamp(idx - 5, 0, maxTarget);
    }
    else
    {
        mScoreOffsetTarget = 0;
    }
    mScoreOffset = static_cast<float>(mScoreOffsetTarget);
}

void MenuComponent::DrawLeaderboardScreen(Render2D& render2D) const
{
    // Layout anchors ported 1:1 from oLeaderboardAPI/Draw_64.gml (non-gxGames).
    // Source _x=72, _y=62. Headers are center-aligned at _x+8/+42/+80; rows are
    // left-aligned at _x/+22/+70 (lines 12-18 + 21 + 34-49). ROUND column from
    // line 18/50 is dropped (no `level` field in the local Entry).
    constexpr float kCenterX = static_cast<float>(kInternalWidth) * 0.5f;
    constexpr float kBaseX = 72.0f;
    constexpr float kPlaceHeaderX = kBaseX + 8.0f;
    constexpr float kNameHeaderX = kBaseX + 42.0f;
    constexpr float kScoreHeaderX = kBaseX + 80.0f;
    constexpr float kRoundHeaderX = kBaseX + 106.0f;
    constexpr float kRowPlaceX = kBaseX;
    constexpr float kRowNameX = kBaseX + 22.0f;
    constexpr float kRowScoreX = kBaseX + 70.0f;
    constexpr float kRowRoundX = kBaseX + 104.0f;
    constexpr float kHeaderY = 62.0f;
    constexpr float kRowStart = 78.0f;
    constexpr float kRowStep = 9.0f;
    // Half-width of the full-alpha band (scoresPerPage/2 - 0.5). Rows farther
    // than this from the viewport center fade per Draw_64.gml:28.
    constexpr float kFadeHalfWidth = 3.5f;

    render2D.DrawText(Font2D::Score, "PLACE", kPlaceHeaderX, kHeaderY, kWhite, TextAlign::Center);
    render2D.DrawText(Font2D::Score, "NAME", kNameHeaderX, kHeaderY, kWhite, TextAlign::Center);
    render2D.DrawText(Font2D::Score, "SCORE", kScoreHeaderX, kHeaderY, kWhite, TextAlign::Center);
    render2D.DrawText(Font2D::Score, "ROUND", kRoundHeaderX, kHeaderY, kWhite, TextAlign::Center);

    if (mDisplayEntries.empty())
    {
        render2D.DrawText(Font2D::Score, "NO SCORES YET", kCenterX, 110.0f, kDkGray, TextAlign::Center);
    }
    else
    {
        // Draw_64.gml:23 iteration range - overshoot the viewport by round(scrollSpd)
        // rows on each side so the smooth Approach can slide rows in/out without
        // popping at the viewport boundaries.
        const int total = static_cast<int>(mDisplayEntries.size());
        const int sp = static_cast<int>(std::round(mScrollSpd));
        const int first = std::max(0, mScoreOffsetTarget - sp);
        const int last = std::min(total, mScoreOffsetTarget + kLeaderboardVisibleRows + sp);
        for (int i = first; i < last; ++i)
        {
            // Draw_64.gml:28 per-row fade: full alpha within kFadeHalfWidth of
            // the viewport center (scoreOffset + 3.5), fading linearly to 0 over
            // the next 1.0 unit, fully transparent beyond.
            const float dist = std::fabs(static_cast<float>(i) - mScoreOffset - kFadeHalfWidth);
            const float alpha = 1.0f - std::clamp(dist - kFadeHalfWidth, 0.0f, 1.0f);

            const bool isPlayer = !mUsername.empty() && mDisplayEntries[i].name == mUsername;
            Graphics::Color rowCol = isPlayer ? kYellow : kWhite;
            rowCol.a = alpha;

            // Draw_64.gml:29 - row Y slides with the smoothed mScoreOffset so the
            // viewport scrolls sub-pixel-fractionally during the Approach.
            const float rowY = kRowStart + (static_cast<float>(i) - mScoreOffset) * kRowStep;
            const int rank = i + 1;

            // Shift the ordinal left as the rank number grows so 4+ digit ranks
            // don't visually crowd the NAME column. Matches Draw_64.gml:31-32's
            // `_scoreX -= 4` at i >= 999 and adds a second -4 at the 5-digit
            // boundary (a port extension; the source has no 10000+ tier).
            float placeX = kRowPlaceX;
            if (rank >= 1000)
            {
                placeX -= 4.0f;
            }
            if (rank >= 10000)
            {
                placeX -= 4.0f;
            }
            render2D.DrawText(Font2D::Score, Ordinal(rank), placeX, rowY, rowCol, TextAlign::Left);
            render2D.DrawText(Font2D::Score, mDisplayEntries[i].name, kRowNameX, rowY, rowCol,
                              TextAlign::Left);
            render2D.DrawText(Font2D::Score, std::to_string(mDisplayEntries[i].score), kRowScoreX, rowY,
                              rowCol, TextAlign::Left);
            render2D.DrawText(Font2D::Score, std::to_string(mDisplayEntries[i].level), kRowRoundX, rowY,
                              rowCol, TextAlign::Left);
        }
    }

    render2D.DrawText(Font2D::Score, "PRESS ENTER TO", kCenterX, 170.0f, kDkGray, TextAlign::Center);
    render2D.DrawText(Font2D::Score, "CONTINUE", kCenterX, 178.0f, kDkGray, TextAlign::Center);
}

void MenuComponent::DebugUI()
{
    ImGui::Text("Menu  option=%d  active=%d  start=%d", mOption, sMenuActive ? 1 : 0, sStartRequested ? 1 : 0);
    ImGui::Text("user='%s'  vol=%.2f  inGame=%d", mUsername.c_str(), mVolume, sInGame ? 1 : 0);
    if (mShowLeaderboard)
    {
        ImGui::Text("[leaderboard screen]");
    }
}
} // namespace Engine::CriticalCore
