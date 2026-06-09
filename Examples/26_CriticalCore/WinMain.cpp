#include <Engine/Inc/Engine.h>
#include "GameState.h"
#include "Leaderboard.h"
#include "LeaderboardFetcher.h"
#include "SelfTest.h"

#include <cstdio>
#include <cstring>

namespace
{
    // Standalone smoke test for the OpenMP-dispatched Firebase fetch + merge
    // (LeaderboardFetcher). No window, no game loop - hits the live REST
    // endpoint, merges every parsed entry into an in-memory local board, and
    // prints the resulting top-10. Returns 0 on success, 1 on network/parse
    // failure. Manual verification path for the multithreaded fetch.
    int RunFetchTest()
    {
        Engine::CriticalCore::Leaderboard board("criticalcore_fetchtest.json");
        board.Load();
        const int localBefore = static_cast<int>(board.Entries().size());

        std::printf("Fetching %s ...\n", Engine::CriticalCore::kRemoteLeaderboardUrl);
        const bool ok = Engine::CriticalCore::FetchAndMergeLeaderboard(
            Engine::CriticalCore::kRemoteLeaderboardUrl, board);
        if (!ok)
        {
            std::printf("FETCH FAILED (no network, curl missing, non-2xx, or parse error)\n");
            return 1;
        }

        std::printf("Local rows before merge: %d\n", localBefore);
        std::printf("Top-10 after merge (name / score / round):\n");
        int rank = 1;
        for (const auto& e : board.Entries())
        {
            std::printf("  %2d. %-12s %10d  r%d\n", rank++, e.name.c_str(), e.score, e.level);
        }
        std::printf("PB (untouched by remote merge): %d\n", board.GetPB());
        return 0;
    }
}

int main(int argc, char** argv)
{
    // Headless self-test mode: if "--selftest" is passed, run every
    // deterministic per-subsystem self-test WITHOUT creating the App/window,
    // GraphicsSystem or GameWorld, then return its exit code (0 = all pass,
    // non-zero = a failure). This is the CI/QA path -- no human, no render.
    // "--fetchtest" runs the live Firebase fetch+merge smoke test (network
    // required, intentionally NOT part of --selftest's deterministic suite).
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i] != nullptr && std::strcmp(argv[i], "--selftest") == 0)
        {
            return Engine::CriticalCore::RunSelfTests();
        }
        if (argv[i] != nullptr && std::strcmp(argv[i], "--fetchtest") == 0)
        {
            return RunFetchTest();
        }
    }

    Engine::App& myApp = Engine::MainApp();
    myApp.AddState<GameState>("Game");

    Engine::AppConfig appConfig;
    appConfig.appName = L"Critical Core 2";
    appConfig.winWidth = 768;
    appConfig.winHeight = 672;
    appConfig.maxVertexCount = 100000;

    myApp.Run(appConfig);
    return 0;
}
