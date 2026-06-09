#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace Engine::CriticalCore
{
    class Leaderboard;

    // One raw row pulled from the Firebase Realtime Database `.json` tree:
    //   { "<name>": { "level": float, "points": float }, ... }
    // Score = points (cast to int). Level = round reached on that run.
    struct RemoteEntry
    {
        std::string name;
        int score = 0;
        int level = 0;
    };

    // Firebase Realtime Database root URL for the live "Critical Core 2" board.
    // The .json suffix is Firebase's REST convention -> GET returns the whole
    // tree as a JSON object:
    //   { "<username>": { "level": float, "points": float }, ... }
    constexpr const char* kRemoteLeaderboardUrl =
        "https://critical-core-2-default-rtdb.firebaseio.com/.json";

    // SYNCHRONOUS fetch + merge. Blocks the calling thread for the duration of
    // the HTTP round-trip + parse. Suitable for the headless `--fetchtest` smoke
    // test and any offline tooling; NOT suitable for the game loop (use the
    // AsyncLeaderboardFetcher below instead). PB is left untouched (remote
    // scores never become your PB). Returns true on success.
    bool FetchAndMergeLeaderboard(const std::string& url, Leaderboard& local);

    // Fire-and-forget background fetch driven by a detached std::thread, polled
    // each tick from the game loop. OpenMP fundamentally cannot model a
    // single-shot async-I/O dispatch without restructuring the entire app
    // around a persistent `#pragma omp parallel` region, so std::thread is the
    // correct primitive here - cheap, well-defined lifetime, no global state.
    //
    // Lifetime: Start() captures a shared_ptr<State> into the worker, so the
    // worker can safely outlive this object (the shared state stays alive
    // until the worker exits). The destructor does NOT join - the worker is
    // detached and self-terminates after writing its result, even if no one
    // ever calls Consume().
    class AsyncLeaderboardFetcher
    {
    public:
        enum class State : int
        {
            Idle = 0,    // never started, or Consume() drained the last result
            Fetching = 1,// background worker in flight
            Ready = 2,   // worker finished, entries waiting for Consume()
            Failed = 3,  // worker finished with an HTTP / parse error
        };

        AsyncLeaderboardFetcher();
        ~AsyncLeaderboardFetcher() = default;

        AsyncLeaderboardFetcher(const AsyncLeaderboardFetcher&) = delete;
        AsyncLeaderboardFetcher& operator=(const AsyncLeaderboardFetcher&) = delete;

        // Kick the worker. NO-OP if a fetch is already Fetching (idempotent
        // mid-flight). Re-starts after Ready/Failed/Idle.
        void Start(const std::string& url);

        // Current state. Cheap atomic load - safe to call every frame.
        State Poll() const;

        // When Poll() == Ready, fold the parsed remote rows into `target` via
        // Leaderboard::MergeRemoteEntry (PB NOT touched). Resets state to Idle
        // and returns the number of rows merged. NO-OP outside Ready (returns 0).
        int Consume(Leaderboard& target);

        // When Poll() == Ready, move the raw (uncapped, unsorted) remote rows
        // into `entriesOut` so the caller can build its own scroll-friendly
        // display buffer instead of paying the Leaderboard top-N trim. Resets
        // state to Idle. NO-OP outside Ready (returns 0).
        int Consume(std::vector<RemoteEntry>& entriesOut);

    private:
        struct SharedState
        {
            std::atomic<int> state{static_cast<int>(State::Idle)};
            std::vector<RemoteEntry> entries;
        };
        std::shared_ptr<SharedState> mState;
    };
}
