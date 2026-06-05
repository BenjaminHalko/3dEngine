#pragma once

namespace Engine::CriticalCore
{
class CoreComponent;
class PlayerComponent;

// ---------------------------------------------------------------------------
// CombatRegistry - decoupled runtime locators the projectiles (Fireball/Spike)
// need but cannot get from the engine (there is NO GameObject iteration / no
// "find component" API). The game FLOW (task 27/34) owns the live Core + Player
// and publishes them here ONCE; projectiles read them. Mirrors the existing
// decoupling patterns in this example (CoreComponent::ConsumeLaunch launch
// registry, EntityComponent's shared gCoreScale, BubbleComponent's static
// SetPlayer/SetBossWalls/SetScoreSink bridges).
//
// HEADER-ONLY: all storage lives in `static` locals of `inline` functions, so a
// single shared instance is linked across every translation unit and NO new
// .cpp / CMakeLists wiring is required.
//
// *** WHY NOT a combatant registry of my own? *** BubbleComponent (task 23) and
// PlayerComponent (task 22) already expose the target-side bridges the Spike
// consumes DIRECTLY:
//   * bubbles : BubbleComponent::AllBubbles() (live list) + bubble->BurstBubble().
//   * player  : the PlayerComponent public API (CenterX/Y, Radius, Mass/SetMass).
// The only things missing are a stable handle to the live Core (for the Fireball)
// and the live Player (for the Spike), plus a score-delta sink - all below.
//
// *** FLOW WIRING CONTRACT (task 27/34 - read before wiring) ***
//   After creating the Core + Player objects:
//     SetActiveCore(core);  SetActivePlayer(player);
//   On teardown: SetActiveCore(nullptr); SetActivePlayer(nullptr).
//   Each fixed step: gameState.score += ConsumeScoreDelta();
// ---------------------------------------------------------------------------

// ===========================================================================
// 1) CoreLocator - the live Core, published once by the flow.
//
// The Fireball reads it to (a) aim at the Core centre, (b) test the live boss
// walls, and (c) call DamageCore() on a boss-wall hit. The Spike reads it for an
// accurate "am I still inside the Core?" guard. When unset (nullptr) the Fireball
// falls back to the arena centre (no DamageCore) and the Spike to the arena
// centre core circle, so both still run standalone.
// ===========================================================================
inline CoreComponent*& ActiveCoreSlot()
{
    static CoreComponent* sActiveCore = nullptr;
    return sActiveCore;
}

inline void SetActiveCore(CoreComponent* core)
{
    ActiveCoreSlot() = core;
}

inline CoreComponent* GetActiveCore()
{
    return ActiveCoreSlot();
}

// ===========================================================================
// 2) PlayerLocator - the live Player, published once by the flow.
//
// The Spike reads it to find an overlapping player (point/radius) and to deal the
// mass penalty (player->SetMass(player->Mass() - amount)). The Fireball reads it
// for the source radius (clamp(player->Radius(), 10, 50)) since the spawning
// Player does not hand the fireball its mass. nullptr => no player interaction.
// ===========================================================================
inline PlayerComponent*& ActivePlayerSlot()
{
    static PlayerComponent* sActivePlayer = nullptr;
    return sActivePlayer;
}

inline void SetActivePlayer(PlayerComponent* player)
{
    ActivePlayerSlot() = player;
}

inline PlayerComponent* GetActivePlayer()
{
    return ActivePlayerSlot();
}

// ===========================================================================
// 3) Score-delta channel - scoring sources accumulate a signed score delta; the
// flow (task 27) drains it each fixed step into global.score (GuiState.score).
// The popup VISUAL stays a runtime-spawned ScoreComponent (oScore); this channel
// only carries the NUMBER (GameMaker's `global.score -= 500`, etc.).
// ===========================================================================
inline int& PendingScoreDeltaSlot()
{
    static int sPendingScoreDelta = 0;
    return sPendingScoreDelta;
}

inline void AddScoreDelta(int delta)
{
    PendingScoreDeltaSlot() += delta;
}

inline int ConsumeScoreDelta()
{
    int delta = PendingScoreDeltaSlot();
    PendingScoreDeltaSlot() = 0;
    return delta;
}
} // namespace Engine::CriticalCore
