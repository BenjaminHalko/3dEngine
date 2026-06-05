#pragma once

#include <string>
#include <vector>

// Round / balance formulas + the per-round Core spawn pattern, ported verbatim from
// "Critical Core 2" (scripts/RoundConfig/RoundConfig.gml). Everything here is PURE
// DATA: no entity-class coupling. The Core entity (task 21) consumes these structs
// and resolves the actual projectiles / angles / random size+speed at spawn time.
//
// Conventions (locked for tasks 21 Core + 27 flow):
//  - All formulas mirror the GML EXACTLY. round is the 1-based global.round.
//    GetCoreStart(1)=0.104, GetCoreStart(5)=0.12; CoreHeal()=0.015;
//    CoreWaitToHeal()=180 frames; CoreSpeedPause()=40 frames;
//    CoreHPDamage(1)=1/4.25; CoreHPDamage denom = 3.5 + round*0.75.
//  - Units: HP is a 0..1 fraction (CoreHPDamage / CoreHeal / CoreTotalDamage operate
//    on hp/targetScale fractions, confirmed via CoreFunctions.gml DamageCore).
//    CoreWaitToHeal / CoreSpeedPause are FRAME counts (fixed 60Hz step, task 14).
//  - CoreShoot returns the DETERMINISTIC structure (angle offsets, counts) RNG-free.
//    Size/speed are returned as RANGES; the Core picks the value with GmHelpers
//    RandomRange at spawn, and also applies the source's per-shot +/-5 deg jitter.
//    Random KIND choices (the GML choose()/irandom() spawns) are resolved here via
//    GmHelpers IRandom/Choose so each ShootSpawn carries a concrete kind.
namespace Engine::CriticalCore
{
//==================================================================================
// Pure round / balance formulas (RoundConfig.gml:1-33)
//==================================================================================

// getCoreStart(): 0.1 + 0.004*round. The Core's starting targetScale per round.
float GetCoreStart(int round);

// getCoreIncrease(): 0.01 + 0.0025*round + 0.04*hpHit. hpHit = oCore.hpHit accumulator.
float GetCoreIncrease(int round, float hpHit);

// coreHPDamage(): 1 / (3.5 + round*0.75). HP fraction removed per fireball hit.
float CoreHPDamage(int round);

// coreHeal(): constant 0.015 HP-fraction regen per heal tick.
float CoreHeal();

// coreWaitToHeal(): 180. Frames the Core waits (after damage) before it starts healing.
float CoreWaitToHeal();

// coreTotalDamage(): hp==0 -> 1; else (1-hp)*targetScale*0.6. targetScale shrink on hit.
float CoreTotalDamage(float hp, float targetScale);

// coreSpeed(): 0.008 - targetScale*0.003. Core dash/move speed (smaller core = faster).
float CoreSpeed(float targetScale);

// coreSpeedPause(): constant 40. Frames the Core pauses between dashes.
float CoreSpeedPause();

//==================================================================================
// Core spawn pattern (coreShoot, RoundConfig.gml:35-97) as DATA
//==================================================================================

enum class ProjectileKind
{
    Bubble,
    Spike
};

// One projectile the Core emits this beat. angleOffset is relative to the Core's
// current shootDir (degrees). If aimAtPlayer is true the offset is ignored and the
// Core resolves point_direction(core, player) at spawn time (the round>=2 even-beat
// homing spike, RoundConfig.gml:92-96). size/spd are RANGES the Core samples with
// RandomRange; spikes carry size 0 (mass unused) and the spike speed range
// (0.4+0.01*round .. 0.8+0.01*round); bubbles carry speed 1.0 .. 1.5.
struct ShootSpawn
{
    float angleOffset;
    ProjectileKind kind;
    float sizeMin;
    float sizeMax;
    float spdMin;
    float spdMax;
    bool aimAtPlayer;
};

// coreShoot(): builds this beat's spawn list for the given round. Angles/counts are
// deterministic; random KIND picks (choose/irandom in the GML) are resolved via
// GmHelpers RNG. round>=2 appends a player-aimed spike on EVEN beats (beatIndex%2==0).
std::vector<ShootSpawn> CoreShoot(int round, int beatIndex);

//==================================================================================
// Self test (asserts rounds 1-6, emits rounds.csv). Task 35's --selftest writes the
// result to .omo/evidence/task-9-rounds.csv.
//==================================================================================
bool RoundConfigSelfTest(std::string& csvOut);
} // namespace Engine::CriticalCore
