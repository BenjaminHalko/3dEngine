#include "LeaderboardFetcher.h"

#include "HttpFetch.h"
#include "Leaderboard.h"

#include <rapidjson/document.h>

#include <cmath>
#include <thread>

namespace Engine::CriticalCore
{
    namespace
    {
        // Read a non-negative integer numeric field. Returns false (skip the
        // row) on missing/NaN/inf/negative; missing-field is treated as 0 by
        // the caller when the field is optional (e.g. "level" on legacy rows).
        bool ReadNonNegativeInt(const rapidjson::Value& row, const char* key, int& out)
        {
            if (!row.IsObject() || !row.HasMember(key))
            {
                return false;
            }
            const rapidjson::Value& v = row[key];
            if (!v.IsNumber())
            {
                return false;
            }
            const double d = v.GetDouble();
            if (std::isnan(d) || std::isinf(d) || d < 0.0)
            {
                return false;
            }
            out = static_cast<int>(d);
            return true;
        }

        // Fetch + parse the Firebase tree into a (name, score, level) list.
        // Returns false on HTTP / parse failure. Shared by the synchronous
        // merge entry point and the async worker. Rows missing "points" are
        // skipped; rows missing "level" default to 0 (legacy data).
        bool FetchRemoteEntries(const std::string& url, std::vector<RemoteEntry>& entriesOut)
        {
            entriesOut.clear();

            std::string body;
            if (!HttpGet(url, body))
            {
                return false;
            }

            rapidjson::Document doc;
            doc.Parse(body.c_str());
            if (doc.HasParseError() || !doc.IsObject())
            {
                return false;
            }

            for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it)
            {
                if (!it->name.IsString())
                {
                    continue;
                }
                const std::string name = it->name.GetString();
                if (name.empty())
                {
                    continue;
                }
                int score = 0;
                if (!ReadNonNegativeInt(it->value, "points", score))
                {
                    continue;
                }
                int level = 0;
                ReadNonNegativeInt(it->value, "level", level); // optional field
                entriesOut.push_back({name, score, level});
            }
            return true;
        }
    }

    bool FetchAndMergeLeaderboard(const std::string& url, Leaderboard& local)
    {
        std::vector<RemoteEntry> entries;
        if (!FetchRemoteEntries(url, entries))
        {
            return false;
        }
        for (const RemoteEntry& e : entries)
        {
            local.MergeRemoteEntry(e.name, e.score, e.level);
        }
        return true;
    }

    AsyncLeaderboardFetcher::AsyncLeaderboardFetcher()
        : mState(std::make_shared<SharedState>())
    {
    }

    void AsyncLeaderboardFetcher::Start(const std::string& url)
    {
        const int current = mState->state.load(std::memory_order_acquire);
        if (current == static_cast<int>(State::Fetching))
        {
            return;
        }
        mState->entries.clear();
        mState->state.store(static_cast<int>(State::Fetching), std::memory_order_release);

        // Capture a shared_ptr copy by value so the worker keeps the SharedState
        // alive even if the owning AsyncLeaderboardFetcher is destroyed before
        // the HTTP round-trip completes. Detached: no join needed - the worker
        // self-terminates after the final store().
        std::shared_ptr<SharedState> state = mState;
        std::thread([state, url]() {
            std::vector<RemoteEntry> entries;
            const bool ok = FetchRemoteEntries(url, entries);
            if (ok)
            {
                state->entries = std::move(entries);
                state->state.store(static_cast<int>(State::Ready), std::memory_order_release);
            }
            else
            {
                state->state.store(static_cast<int>(State::Failed), std::memory_order_release);
            }
        }).detach();
    }

    AsyncLeaderboardFetcher::State AsyncLeaderboardFetcher::Poll() const
    {
        return static_cast<State>(mState->state.load(std::memory_order_acquire));
    }

    int AsyncLeaderboardFetcher::Consume(Leaderboard& target)
    {
        if (Poll() != State::Ready)
        {
            return 0;
        }
        int merged = 0;
        for (const RemoteEntry& e : mState->entries)
        {
            target.MergeRemoteEntry(e.name, e.score, e.level);
            ++merged;
        }
        mState->entries.clear();
        mState->state.store(static_cast<int>(State::Idle), std::memory_order_release);
        return merged;
    }

    int AsyncLeaderboardFetcher::Consume(std::vector<RemoteEntry>& entriesOut)
    {
        if (Poll() != State::Ready)
        {
            return 0;
        }
        const int count = static_cast<int>(mState->entries.size());
        entriesOut = std::move(mState->entries);
        mState->entries.clear();
        mState->state.store(static_cast<int>(State::Idle), std::memory_order_release);
        return count;
    }
}
