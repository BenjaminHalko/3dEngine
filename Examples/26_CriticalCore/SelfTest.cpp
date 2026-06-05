#include "SelfTest.h"

#include "AnimCurve.h"
#include "CameraShakeService.h"
#include "Collision.h"
#include "GameClock.h"
#include "GmHelpers.h"
#include "Leaderboard.h"
#include "MusicController.h"
#include "RoundConfig.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace Engine::CriticalCore
{
namespace
{
namespace fs = std::filesystem;

// Run cwd is build/bin; the evidence dir lives at the repo root (.omo/evidence).
// Walk up from cwd looking for an ancestor that already contains .omo so the
// path resolves regardless of how deep build/bin sits. Fall back to the
// canonical ../../.omo/evidence relative to build/bin.
fs::path ResolveEvidenceDir()
{
    std::error_code ec;
    fs::path cur = fs::current_path(ec);
    if (!ec)
    {
        for (int i = 0; i < 8; ++i)
        {
            if (fs::exists(cur / ".omo", ec))
            {
                return cur / ".omo" / "evidence";
            }
            if (!cur.has_parent_path())
            {
                break;
            }
            cur = cur.parent_path();
        }
    }
    return fs::path("..") / ".." / ".omo" / "evidence";
}

bool WriteArtifact(const fs::path& dir, const std::string& name, const std::string& content)
{
    std::error_code ec;
    fs::create_directories(dir, ec);
    const fs::path full = dir / name;
    std::ofstream out(full, std::ios::binary | std::ios::trunc);
    if (!out)
    {
        return false;
    }
    out << content;
    return out.good();
}

// One row of the summary table.
struct TestResult
{
    std::string name;     // subsystem label
    std::string artifact; // artifact filename(s), or "-" for N/A
    bool ran = false;     // false => N/A (skipped honestly, never a pass)
    bool passed = false;  // only meaningful when ran == true
};
} // namespace

int RunSelfTests()
{
    const fs::path evidenceDir = ResolveEvidenceDir();

    std::printf("==================================================\n");
    std::printf(" Critical Core 2 (Example 26) -- headless selftest\n");
    std::printf(" evidence dir: %s\n", evidenceDir.string().c_str());
    std::printf("==================================================\n");

    // Seed the shared RNG up front so every randomness-using test is
    // reproducible (individual tests reseed too, but this guards any that
    // do not).
    SeedRng(0xC0FFEEu);

    std::vector<TestResult> results;

    // Helper: run a single-artifact test, capture + write its output, record it.
    auto runOne = [&](const std::string& name, const std::string& artifact, bool ok, const std::string& content)
    {
        TestResult r;
        r.name = name;
        r.artifact = artifact;
        r.ran = true;
        r.passed = ok;
        if (!WriteArtifact(evidenceDir, artifact, content))
        {
            std::printf("  ! could not write artifact %s\n", artifact.c_str());
            r.passed = false; // an unwritten artifact is a failure of this run
        }
        results.push_back(r);
    };

    // 1. GameClock fixed-step accumulator (static member self-test).
    {
        std::string csv;
        const bool ok = GameClock::SelfTest(csv);
        runOne("GameClock", "task-14-clock.csv", ok, csv);
    }

    // 2. GmHelpers math/easing/color/RNG (free function).
    {
        SeedRng(0xC0FFEEu);
        std::string csv;
        const bool ok = HelpersSelfTest(csv);
        runOne("GmHelpers", "task-8-helpers.csv", ok, csv);
    }

    // 3. RoundConfig round/balance formulas + coreShoot patterns (free fn,
    //    seeds 0xC0FFEE internally).
    {
        std::string csv;
        const bool ok = RoundConfigSelfTest(csv);
        runOne("RoundConfig", "task-9-rounds.csv", ok, csv);
    }

    // 4. AnimCurve baked LUT (class member self-test).
    {
        AnimCurve curve;
        std::string csv;
        const bool ok = curve.SelfTest(csv);
        runOne("AnimCurve", "task-5-animcurve.csv", ok, csv);
    }

    // 5. Collision octagon-arena reflection + death predicate (free fn, two
    //    string out-params: collision CSV + death log).
    {
        SeedRng(0xC0FFEEu);
        std::string collideCsv;
        std::string deathLog;
        const bool ok = CollisionSelfTest(collideCsv, deathLog);
        TestResult r;
        r.name = "Collision";
        r.artifact = "task-17-collide.csv + task-17-death.txt";
        r.ran = true;
        r.passed = ok;
        if (!WriteArtifact(evidenceDir, "task-17-collide.csv", collideCsv) ||
            !WriteArtifact(evidenceDir, "task-17-death.txt", deathLog))
        {
            std::printf("  ! could not write Collision artifacts\n");
            r.passed = false;
        }
        results.push_back(r);
    }

    // 6. CameraShakeService screenshake envelope (class member self-test).
    //    Stack-constructed: ShakeSelfTest is purely internal (never touches the
    //    owning GameWorld), so no service registration is required.
    {
        SeedRng(0xC0FFEEu);
        CameraShakeService shake;
        std::string csv;
        const bool ok = shake.ShakeSelfTest(csv);
        runOne("CameraShake", "task-19-shake.csv", ok, csv);
    }

    // 7. MusicController beat clock (free fn; drives ComputeBeat off a synthetic
    //    60Hz cursor ramp, no audio device needed).
    {
        std::string csv;
        const bool ok = BeatSelfTest(csv);
        runOne("MusicBeat", "task-18-beatlog.csv", ok, csv);
    }

    // 8. Leaderboard local JSON board (free fn; uses a temp save file).
    {
        std::string txt;
        const bool ok = LeaderboardSelfTest(txt);
        runOne("Leaderboard", "task-30-leaderboard.txt", ok, txt);
    }

    // N/A: these subsystems have NO callable logic-only self-test -- their
    // behaviour is entangled with a live render pass / populated GameWorld
    // (transforms, services, sprites). Reported honestly; never a pass, never a
    // fail. Adding a meaningful check would require standing up the engine.
    for (const char* na : {"EntityComponent", "CoreComponent/coreShoot", "PlayerComponent",
                           "BubbleComponent", "Fireball/Spike projectiles", "Trail/Sparkle particles"})
    {
        TestResult r;
        r.name = na;
        r.artifact = "-";
        r.ran = false;
        r.passed = false;
        results.push_back(r);
    }

    // ---- summary table ----
    int passCount = 0;
    int failCount = 0;
    int naCount = 0;

    std::printf("\n");
    std::printf("+----------------------------+-------------------------------------------+--------+\n");
    std::printf("| %-26s | %-41s | %-6s |\n", "SUBSYSTEM", "ARTIFACT", "RESULT");
    std::printf("+----------------------------+-------------------------------------------+--------+\n");
    for (const TestResult& r : results)
    {
        const char* status;
        if (!r.ran)
        {
            status = "N/A";
            ++naCount;
        }
        else if (r.passed)
        {
            status = "PASS";
            ++passCount;
        }
        else
        {
            status = "FAIL";
            ++failCount;
        }
        std::printf("| %-26s | %-41s | %-6s |\n", r.name.c_str(), r.artifact.c_str(), status);
    }
    std::printf("+----------------------------+-------------------------------------------+--------+\n");
    std::printf(" %d passed, %d failed, %d N/A (needs live world)\n", passCount, failCount, naCount);

    if (failCount > 0)
    {
        std::printf("\nFAILURES:\n");
        for (const TestResult& r : results)
        {
            if (r.ran && !r.passed)
            {
                std::printf("  - %s (%s) FAILED an internal assertion\n", r.name.c_str(), r.artifact.c_str());
            }
        }
        std::printf("\nSELFTEST RESULT: FAIL (%d subsystem(s) failed)\n", failCount);
        return failCount; // non-zero exit
    }

    std::printf("\nSELFTEST RESULT: ALL PASS\n");
    return 0;
}
} // namespace Engine::CriticalCore
