#pragma once

#include <string>
#include <vector>

namespace Engine::CriticalCore
{
    // Local JSON-backed highscore leaderboard (replaces the cut online/Firebase
    // board). Persists settings + personal best + a top-10 score list to a JSON
    // file in the working directory (build/bin). Missing/corrupt files degrade
    // gracefully to an empty, default state — never throw, never crash.
    //
    // Schema (criticalcore_save.json):
    //   {
    //     "username": str,
    //     "volume":   float,   // 0..1
    //     "render":   bool,
    //     "pb":       int,
    //     "scores":   [ { "name": str, "score": int }, ... up to 10 ]
    //   }
    class Leaderboard
    {
    public:
        // One row of the top-10 board.
        struct Entry
        {
            std::string name;
            int score = 0;
        };

        // Default save file name (relative to cwd == build/bin). The selftest
        // may use a different path to avoid clobbering a real save.
        static constexpr const char* kDefaultPath = "criticalcore_save.json";

        explicit Leaderboard(std::string filePath = kDefaultPath);

        // Reads the JSON file into memory. On file-not-found, parse error, or
        // missing/mistyped keys, the offending value falls back to its default
        // and loading continues — the object is always left in a valid state.
        void Load();

        // Writes the current settings + PB + scores back to the JSON file.
        void Save();

        // --- Settings accessors (persisted across Save/Load) ---
        const std::string& GetUsername() const;
        void SetUsername(const std::string& name); // clamped to <=10 chars
        float GetVolume() const;
        void SetVolume(float volume); // clamped to [0,1]
        bool GetRender() const;
        void SetRender(bool render);
        int GetPB() const;

        // == GML LeaderboardPost: inserts the run, sorts DESC by score, trims to
        // top-10, and bumps PB when score > pb.
        void Post(const std::string& name, int score);

        // The top-10 rows, sorted DESC by score (for the post-game display).
        const std::vector<Entry>& Entries() const;

        // Index into Entries() of the player's run (name + score match), for
        // highlighting. Returns -1 when not present.
        int PlayerRowIndex(const std::string& name, int score) const;

    private:
        void Reset();

        std::string mFilePath;
        std::string mUsername;
        float mVolume = 0.7f;
        bool mRender = true;
        int mPB = 0;
        std::vector<Entry> mScores;
    };

    // Self-test (task 35 -> .omo/evidence/task-30-leaderboard.txt). Exercises a
    // Post/Save/reload round-trip (order + PB) and corrupt-file recovery.
    bool LeaderboardSelfTest(std::string& out);
}
