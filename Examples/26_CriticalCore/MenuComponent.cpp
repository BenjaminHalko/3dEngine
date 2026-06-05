#include "MenuComponent.h"

#include "GmHelpers.h"
#include "Render2D.h"

#include <algorithm>
#include <cmath>
#include <string>

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

// GameMaker named colors used by the menu.
const Graphics::Color kWhite = Graphics::Colors::White;
const Graphics::Color kRed = Graphics::Colors::Red;
const Graphics::Color kDkGray = Graphics::Color{0.25f, 0.25f, 0.25f, 1.0f}; // c_dkgray (64,64,64)
const Graphics::Color kHighlight = Graphics::Color{0.38f, 1.0f, 0.63f, 1.0f}; // #61FFA0 player row

// Convenience: one-shot key press this fixed step.
bool Pressed(KeyCode key)
{
    const InputSystem* in = InputSystem::Get();
    return (in != nullptr) && in->IsKeyPressed(key);
}

bool Held(KeyCode key)
{
    const InputSystem* in = InputSystem::Get();
    return (in != nullptr) && in->IsKeyDown(key);
}
} // namespace

// Shared single-menu signals.
bool MenuComponent::sMenuActive = false;
bool MenuComponent::sStartRequested = false;

bool MenuComponent::IsMenuActive()
{
    return sMenuActive;
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
    mRender = mLeaderboard.GetRender();

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

    // Once START fires the menu stops responding (the flow destroys this object).
    if (!sMenuActive)
    {
        return;
    }

    // Leaderboard sub-screen: ENTER or ESC returns to the menu.
    if (mShowLeaderboard)
    {
        if (Pressed(KeyCode::RETURN) || Pressed(KeyCode::ESCAPE))
        {
            mShowLeaderboard = false;
        }
        mUsernameFlash = Approach(mUsernameFlash, 0.0f, 0.04f);
        return;
    }

    // Render/FX toggle (global.render). Bound to TAB so it never collides with
    // username typing; persisted immediately.
    if (Pressed(KeyCode::TAB))
    {
        mRender = !mRender;
        mLeaderboard.SetRender(mRender);
        mLeaderboard.Save();
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
    const bool up = Pressed(KeyCode::UP) || (allowLetters && Pressed(KeyCode::W));
    const bool down = Pressed(KeyCode::DOWN) || (allowLetters && Pressed(KeyCode::S));

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
    }

    const bool select = Pressed(KeyCode::RETURN);

    if (mOption == kOptStart && select)
    {
        // Step_0.gml:31-48 (online branches dropped): require a username, then
        // raise the start signal the flow polls. The flow performs GameStart
        // (play music, snStart SFX, transition + destroy this object).
        if (!mUsername.empty())
        {
            sStartRequested = true;
            sMenuActive = false;
        }
        else
        {
            // No name -> red flash prompt (Step_0.gml:45).
            mUsernameFlash = 1.0f;
        }
    }
    else if (mOption == kOptLeaderboard && select)
    {
        // Step_0.gml:50-53 GotoLeaderboard, now a local in-place screen.
        mLeaderboard.Load(); // refresh to show the latest board
        mShowLeaderboard = true;
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
    if (Pressed(KeyCode::BACKSPACE) && !mUsername.empty())
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
        if (Pressed(static_cast<KeyCode>(c)))
        {
            const char ch = shift ? static_cast<char>(c)
                                  : static_cast<char>(c - 'A' + 'a');
            mUsername.push_back(ch);
        }
    }

    // Digits 0..9.
    for (int c = '0'; c <= '9'; ++c)
    {
        if (Pressed(static_cast<KeyCode>(c)))
        {
            mUsername.push_back(static_cast<char>(c));
        }
    }

    // Space allowed only after at least one character (Step_0.gml:57 vk_space guard).
    if (Pressed(KeyCode::SPACE) && !mUsername.empty())
    {
        mUsername.push_back(' ');
    }
}

void MenuComponent::UpdateVolume()
{
    // LEFT/RIGHT (and A/D) adjust the slider (Step_0.gml:72-84).
    const bool left = Pressed(KeyCode::LEFT) || Pressed(KeyCode::A);
    const bool right = Pressed(KeyCode::RIGHT) || Pressed(KeyCode::D);

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

    // Once START fires the flow owns the screen; draw nothing until destroyed.
    if (!sMenuActive)
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
    render2D.DrawSprite(mAudioLineFillTex, knobX, menuY, 0.0f, 2.0f, 1.0f, 1.0f, 0.0f, kWhite);
    render2D.DrawSprite(mAudioIconTex, kMenuX + 62.0f, menuY, 0.0f, 3.0f, 1.0f, 1.0f, 0.0f, kWhite);

    // Render/FX toggle indicator (TAB) — not in the GML menu, but the brief
    // requires an in-menu render toggle; shown compactly beneath the slider.
    render2D.DrawText(Font2D::Font, mRender ? "FX ON" : "FX OFF", kMenuX, menuY + 10.0f, kWhite);
}

void MenuComponent::DrawLeaderboardScreen(Render2D& render2D) const
{
    // Local board (task 30) — replaces the cut online leaderboard screen.
    render2D.DrawText(Font2D::Font, "LEADERBOARD", kInternalWidth * 0.5f, 14.0f, kWhite, TextAlign::Center);

    const std::vector<Leaderboard::Entry>& entries = mLeaderboard.Entries();
    if (entries.empty())
    {
        render2D.DrawText(Font2D::Score, "NO SCORES YET", kInternalWidth * 0.5f, 100.0f, kDkGray, TextAlign::Center);
    }
    else
    {
        float rowY = 36.0f;
        for (std::size_t i = 0; i < entries.size(); ++i)
        {
            // Highlight the player's own rows by username match (the exact-run
            // PlayerRowIndex highlight is for the post-game board, not the menu).
            const bool isPlayer = !mUsername.empty() && entries[i].name == mUsername;
            const Graphics::Color rowCol = isPlayer ? kHighlight : kWhite;

            const std::string rank = std::to_string(i + 1) + ". " + entries[i].name;
            render2D.DrawText(Font2D::Score, rank, 40.0f, rowY, rowCol, TextAlign::Left);
            render2D.DrawText(Font2D::Score, std::to_string(entries[i].score), 216.0f, rowY, rowCol, TextAlign::Right);
            rowY += 14.0f;
        }
    }

    render2D.DrawText(Font2D::Font, "PRESS ENTER", kInternalWidth * 0.5f, 208.0f, kWhite, TextAlign::Center);
}

void MenuComponent::DebugUI()
{
    ImGui::Text("Menu  option=%d  active=%d  start=%d", mOption, sMenuActive ? 1 : 0, sStartRequested ? 1 : 0);
    ImGui::Text("user='%s'  vol=%.2f  fx=%d", mUsername.c_str(), mVolume, mRender ? 1 : 0);
    if (mShowLeaderboard)
    {
        ImGui::Text("[leaderboard screen]");
    }
}
} // namespace Engine::CriticalCore
