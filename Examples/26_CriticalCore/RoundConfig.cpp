#include "RoundConfig.h"

#include "GmHelpers.h"

#include <cmath>
#include <sstream>

namespace Engine::CriticalCore
{
//==================================================================================
// Pure round / balance formulas (RoundConfig.gml:1-33)
//==================================================================================

float GetCoreStart(int round)
{
    // getCoreStart(): return 0.1 + 0.004 * global.round; (RoundConfig.gml:1-3)
    return 0.1f + 0.004f * static_cast<float>(round);
}

float GetCoreIncrease(int round, float hpHit)
{
    // getCoreIncrease(): 0.01 + 0.0025*round + 0.04*oCore.hpHit; (RoundConfig.gml:5-7)
    return 0.01f + 0.0025f * static_cast<float>(round) + 0.04f * hpHit;
}

float CoreHPDamage(int round)
{
    // coreHPDamage(): 1 / (3.5 + global.round * 0.75); (RoundConfig.gml:9-11)
    return 1.0f / (3.5f + static_cast<float>(round) * 0.75f);
}

float CoreHeal()
{
    // coreHeal(): return 0.015; (RoundConfig.gml:13-15)
    return 0.015f;
}

float CoreWaitToHeal()
{
    // coreWaitToHeal(): return 180; (RoundConfig.gml:17-19) -- frame count.
    return 180.0f;
}

float CoreTotalDamage(float hp, float targetScale)
{
    // coreTotalDamage(): hp==0 -> 1; else (1-hp)*targetScale*0.6; (RoundConfig.gml:21-25)
    if (hp == 0.0f)
    {
        return 1.0f;
    }
    return (1.0f - hp) * targetScale * 0.6f;
}

float CoreSpeed(float targetScale)
{
    // coreSpeed(): 0.008 - oCore.targetScale * 0.003; (RoundConfig.gml:27-29)
    return 0.008f - targetScale * 0.003f;
}

float CoreSpeedPause()
{
    // coreSpeedPause(): return 40; (RoundConfig.gml:31-33) -- frame count.
    return 40.0f;
}

//==================================================================================
// Core spawn pattern (coreShoot, RoundConfig.gml:35-97)
//==================================================================================

namespace
{
// Mirrors the GML inner `_shoot(_shootDir, _obj, _size)` helper (RoundConfig.gml:36-53):
// the per-shot +/-5 deg direction jitter and the RandomRange size/speed sampling are
// deferred to the Core (kept as ranges); spikes ignore size (mass) and use the spike
// speed range 0.4+0.01*round .. 0.8+0.01*round, bubbles use 1.0 .. 1.5.
ShootSpawn MakeShoot(float offset, ProjectileKind kind, float sizeLo, float sizeHi, int round)
{
    ShootSpawn s;
    s.angleOffset = offset;
    s.kind = kind;
    s.aimAtPlayer = false;
    if (kind == ProjectileKind::Spike)
    {
        s.sizeMin = 0.0f;
        s.sizeMax = 0.0f;
        s.spdMin = 0.4f + 0.01f * static_cast<float>(round);
        s.spdMax = 0.8f + 0.01f * static_cast<float>(round);
    }
    else
    {
        s.sizeMin = sizeLo;
        s.sizeMax = sizeHi;
        s.spdMin = 1.0f;
        s.spdMax = 1.5f;
    }
    return s;
}
} // namespace

std::vector<ShootSpawn> CoreShoot(int round, int beatIndex)
{
    std::vector<ShootSpawn> out;

    // Per-round switch (RoundConfig.gml:55-90). Angles/counts deterministic; the
    // choose()/irandom() KIND picks are resolved via GmHelpers RNG.
    switch (round)
    {
    case 1:
    {
        out.push_back(MakeShoot(0.0f, ProjectileKind::Bubble, 100.0f, 150.0f, round));
        out.push_back(MakeShoot(180.0f, ProjectileKind::Bubble, 100.0f, 150.0f, round));
    }
    break;
    case 2:
    {
        out.push_back(MakeShoot(0.0f, ProjectileKind::Bubble, 100.0f, 120.0f, round));
        out.push_back(MakeShoot(120.0f, ProjectileKind::Bubble, 100.0f, 120.0f, round));
        out.push_back(MakeShoot(240.0f, ProjectileKind::Bubble, 100.0f, 120.0f, round));
    }
    break;
    case 3:
    {
        out.push_back(MakeShoot(0.0f, ProjectileKind::Bubble, 90.0f, 110.0f, round));
        // _shoot(shootDir+180, choose(oSpike, oBubble));  size defaults to 0.
        out.push_back(MakeShoot(180.0f, Choose<ProjectileKind>({ProjectileKind::Spike, ProjectileKind::Bubble}), 0.0f, 0.0f, round));
        out.push_back(MakeShoot(90.0f, ProjectileKind::Bubble, 100.0f, 150.0f, round));
        out.push_back(MakeShoot(270.0f, ProjectileKind::Bubble, 100.0f, 150.0f, round));
    }
    break;
    case 4:
    {
        out.push_back(MakeShoot(0.0f, ProjectileKind::Bubble, 220.0f, 250.0f, round));
        out.push_back(MakeShoot(180.0f, ProjectileKind::Bubble, 220.0f, 250.0f, round));
    }
    break;
    default:
    {
        if (round % 3 == 0)
        {
            out.push_back(MakeShoot(0.0f, ProjectileKind::Bubble, 60.0f, 130.0f, round));
            // _shoot(shootDir+180, choose(oBubble, oSpike), random_range(100,130));
            out.push_back(MakeShoot(180.0f, Choose<ProjectileKind>({ProjectileKind::Bubble, ProjectileKind::Spike}), 100.0f, 130.0f, round));
            out.push_back(MakeShoot(90.0f, ProjectileKind::Bubble, 60.0f, 130.0f, round));
            out.push_back(MakeShoot(270.0f, ProjectileKind::Bubble, 60.0f, 130.0f, round));
        }
        else if (round % 3 == 1)
        {
            // irandom(5) == 0 ? oSpike : oBubble  (1-in-6 spike)
            out.push_back(MakeShoot(0.0f, IRandom(5) == 0 ? ProjectileKind::Spike : ProjectileKind::Bubble, 200.0f, 220.0f, round));
            out.push_back(MakeShoot(180.0f, IRandom(5) == 0 ? ProjectileKind::Spike : ProjectileKind::Bubble, 200.0f, 220.0f, round));
        }
        else
        {
            // irandom(4) == 0 ? oSpike : oBubble  (1-in-5 spike)
            out.push_back(MakeShoot(0.0f, IRandom(4) == 0 ? ProjectileKind::Spike : ProjectileKind::Bubble, 130.0f, 180.0f, round));
            out.push_back(MakeShoot(120.0f, ProjectileKind::Bubble, 100.0f, 140.0f, round));
            out.push_back(MakeShoot(240.0f, IRandom(4) == 0 ? ProjectileKind::Spike : ProjectileKind::Bubble, 150.0f, 180.0f, round));
        }
    }
    break;
    }

    // round>=2: aim a spike at the player every 2 beats (even beats). (RoundConfig.gml:92-96)
    if (round >= 2 && (beatIndex % 2 == 0))
    {
        ShootSpawn aimed = MakeShoot(0.0f, ProjectileKind::Spike, 0.0f, 0.0f, round);
        aimed.aimAtPlayer = true;
        out.push_back(aimed);
    }

    return out;
}

//==================================================================================
// Self test
//==================================================================================
namespace
{
bool Approx(float a, float b, float eps = 1e-4f)
{
    return std::fabs(a - b) <= eps;
}
} // namespace

bool RoundConfigSelfTest(std::string& csvOut)
{
    SeedRng(0xC0FFEE);

    std::ostringstream csv;
    csv << "name,expected,actual,pass\n";
    bool allPass = true;

    auto checkF = [&](const std::string& name, float expected, float actual)
    {
        const bool pass = Approx(expected, actual);
        allPass = allPass && pass;
        csv << name << ',' << expected << ',' << actual << ',' << (pass ? 1 : 0) << '\n';
    };
    auto checkI = [&](const std::string& name, long expected, long actual)
    {
        const bool pass = expected == actual;
        allPass = allPass && pass;
        csv << name << ',' << expected << ',' << actual << ',' << (pass ? 1 : 0) << '\n';
    };

    // --- Pure formulas ---
    checkF("GetCoreStart(1)", 0.104f, GetCoreStart(1));
    checkF("GetCoreStart(5)", 0.12f, GetCoreStart(5));
    checkF("CoreHPDamage(1)=1/4.25", 1.0f / 4.25f, CoreHPDamage(1));
    checkF("CoreHeal()", 0.015f, CoreHeal());
    checkF("CoreWaitToHeal()", 180.0f, CoreWaitToHeal());
    checkF("CoreSpeedPause()", 40.0f, CoreSpeedPause());
    checkF("CoreTotalDamage(0,x)=1", 1.0f, CoreTotalDamage(0.0f, 0.5f));
    checkF("CoreTotalDamage(0.5,0.5)", (1.0f - 0.5f) * 0.5f * 0.6f, CoreTotalDamage(0.5f, 0.5f));
    checkF("CoreSpeed(0.5)", 0.008f - 0.5f * 0.003f, CoreSpeed(0.5f));
    checkF("GetCoreIncrease(2,1)", 0.01f + 0.0025f * 2.0f + 0.04f * 1.0f, GetCoreIncrease(2, 1.0f));

    // --- round 1: 2 bubbles at {0,180}, no homing spike (round<2). beatIndex even. ---
    {
        const auto r1 = CoreShoot(1, 0);
        checkI("round1.count", 2, static_cast<long>(r1.size()));
        if (r1.size() == 2)
        {
            checkF("round1[0].angle", 0.0f, r1[0].angleOffset);
            checkF("round1[1].angle", 180.0f, r1[1].angleOffset);
            checkI("round1[0].bubble", 0, static_cast<long>(r1[0].kind));
            checkI("round1[1].bubble", 0, static_cast<long>(r1[1].kind));
            checkI("round1.noAimedSpike", 0, r1[1].aimAtPlayer ? 1 : 0);
        }
    }

    // --- round 2 odd beat: 3 bubbles at {0,120,240}, no homing spike. ---
    {
        const auto r2 = CoreShoot(2, 1);
        checkI("round2.count", 3, static_cast<long>(r2.size()));
        if (r2.size() == 3)
        {
            checkF("round2[0].angle", 0.0f, r2[0].angleOffset);
            checkF("round2[1].angle", 120.0f, r2[1].angleOffset);
            checkF("round2[2].angle", 240.0f, r2[2].angleOffset);
            checkI("round2[0].bubble", 0, static_cast<long>(r2[0].kind));
            checkI("round2[2].bubble", 0, static_cast<long>(r2[2].kind));
        }
    }

    // --- round 2 even beat: appends a player-aimed spike (count 3+1). ---
    {
        const auto r2e = CoreShoot(2, 0);
        checkI("round2even.count", 4, static_cast<long>(r2e.size()));
        if (!r2e.empty())
        {
            const ShootSpawn& last = r2e.back();
            checkI("round2even.aimedSpike.kind", 1, static_cast<long>(last.kind));
            checkI("round2even.aimedSpike.aim", 1, last.aimAtPlayer ? 1 : 0);
        }
    }

    // --- round 3: 4 shots at {0,180,90,270}. ---
    {
        const auto r3 = CoreShoot(3, 1);
        checkI("round3.count", 4, static_cast<long>(r3.size()));
        if (r3.size() == 4)
        {
            checkF("round3[0].angle", 0.0f, r3[0].angleOffset);
            checkF("round3[1].angle", 180.0f, r3[1].angleOffset);
            checkF("round3[2].angle", 90.0f, r3[2].angleOffset);
            checkF("round3[3].angle", 270.0f, r3[3].angleOffset);
        }
    }

    // --- round 4: 2 bubbles at {0,180}, big size 220-250. ---
    {
        const auto r4 = CoreShoot(4, 1);
        checkI("round4.count", 2, static_cast<long>(r4.size()));
        if (r4.size() == 2)
        {
            checkF("round4[0].angle", 0.0f, r4[0].angleOffset);
            checkF("round4[1].angle", 180.0f, r4[1].angleOffset);
            checkF("round4[0].sizeMin", 220.0f, r4[0].sizeMin);
            checkF("round4[0].sizeMax", 250.0f, r4[0].sizeMax);
        }
    }

    // --- round 5: default, round%3==2 -> 3 shots at {0,120,240}. ---
    {
        const auto r5 = CoreShoot(5, 1);
        checkI("round5.count", 3, static_cast<long>(r5.size()));
        if (r5.size() == 3)
        {
            checkF("round5[0].angle", 0.0f, r5[0].angleOffset);
            checkF("round5[1].angle", 120.0f, r5[1].angleOffset);
            checkF("round5[2].angle", 240.0f, r5[2].angleOffset);
        }
    }

    // --- round 6: default, round%3==0 -> 4 shots at {0,180,90,270}. ---
    {
        const auto r6 = CoreShoot(6, 1);
        checkI("round6.count", 4, static_cast<long>(r6.size()));
        if (r6.size() == 4)
        {
            checkF("round6[0].angle", 0.0f, r6[0].angleOffset);
            checkF("round6[1].angle", 180.0f, r6[1].angleOffset);
            checkF("round6[2].angle", 90.0f, r6[2].angleOffset);
            checkF("round6[3].angle", 270.0f, r6[3].angleOffset);
        }
    }

    csv << "RESULT," << (allPass ? "PASS" : "FAIL") << ",,\n";
    csvOut = csv.str();
    return allPass;
}
} // namespace Engine::CriticalCore
