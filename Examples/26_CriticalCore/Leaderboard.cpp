#include "Leaderboard.h"

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/writer.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <sstream>

namespace Engine::CriticalCore
{
    namespace
    {
        constexpr int kMaxScores = 10;
        constexpr int kMaxUsernameLen = 10;

        // Cross-platform file open shim (mirrors Framework/Core/Inc/Common.h's
        // fopen_s). Returns nullptr on failure.
        FILE* OpenFile(const char* path, const char* mode)
        {
            FILE* file = nullptr;
#if defined(_WIN32)
            if (fopen_s(&file, path, mode) != 0)
            {
                return nullptr;
            }
#else
            file = std::fopen(path, mode);
#endif
            return file;
        }
    }

    Leaderboard::Leaderboard(std::string filePath)
        : mFilePath(std::move(filePath))
    {
    }

    void Leaderboard::Reset()
    {
        mUsername.clear();
        mVolume = 0.7f;
        mRender = true;
        mPB = 0;
        mScores.clear();
    }

    void Leaderboard::Load()
    {
        // Always begin from a clean, valid default state so any partial/corrupt
        // read leaves the board usable.
        Reset();

        FILE* file = OpenFile(mFilePath.c_str(), "rb");
        if (file == nullptr)
        {
            // Missing file -> default-empty state, no crash.
            return;
        }

        char readBuffer[65536];
        rapidjson::FileReadStream readStream(file, readBuffer, sizeof(readBuffer));

        rapidjson::Document doc;
        doc.ParseStream(readStream);
        std::fclose(file);

        if (doc.HasParseError() || !doc.IsObject())
        {
            // Corrupt/garbage file -> default-empty state, no crash.
            Reset();
            return;
        }

        if (doc.HasMember("username") && doc["username"].IsString())
        {
            mUsername = doc["username"].GetString();
            if (static_cast<int>(mUsername.size()) > kMaxUsernameLen)
            {
                mUsername.clear();
            }
        }
        if (doc.HasMember("volume") && doc["volume"].IsNumber())
        {
            mVolume = std::clamp(doc["volume"].GetFloat(), 0.0f, 1.0f);
        }
        if (doc.HasMember("render") && doc["render"].IsBool())
        {
            mRender = doc["render"].GetBool();
        }
        if (doc.HasMember("pb") && doc["pb"].IsInt())
        {
            mPB = doc["pb"].GetInt();
        }

        if (doc.HasMember("scores") && doc["scores"].IsArray())
        {
            for (const auto& row : doc["scores"].GetArray())
            {
                if (!row.IsObject())
                {
                    continue;
                }
                Entry entry;
                if (row.HasMember("name") && row["name"].IsString())
                {
                    entry.name = row["name"].GetString();
                }
                if (row.HasMember("score") && row["score"].IsInt())
                {
                    entry.score = row["score"].GetInt();
                }
                mScores.push_back(entry);
            }
        }

        // Defensive: enforce the sorted/trimmed invariant even on hand-edited
        // files so downstream display code can trust the order.
        std::stable_sort(
            mScores.begin(), mScores.end(), [](const Entry& a, const Entry& b) { return a.score > b.score; });
        if (static_cast<int>(mScores.size()) > kMaxScores)
        {
            mScores.resize(kMaxScores);
        }
    }

    void Leaderboard::Save()
    {
        FILE* file = OpenFile(mFilePath.c_str(), "wb");
        if (file == nullptr)
        {
            return;
        }

        char writeBuffer[65536];
        rapidjson::FileWriteStream writeStream(file, writeBuffer, sizeof(writeBuffer));
        rapidjson::Writer<rapidjson::FileWriteStream> writer(writeStream);

        writer.StartObject();
        writer.Key("username");
        writer.String(mUsername.c_str(), static_cast<rapidjson::SizeType>(mUsername.size()));
        writer.Key("volume");
        writer.Double(static_cast<double>(mVolume));
        writer.Key("render");
        writer.Bool(mRender);
        writer.Key("pb");
        writer.Int(mPB);
        writer.Key("scores");
        writer.StartArray();
        for (const Entry& entry : mScores)
        {
            writer.StartObject();
            writer.Key("name");
            writer.String(entry.name.c_str(), static_cast<rapidjson::SizeType>(entry.name.size()));
            writer.Key("score");
            writer.Int(entry.score);
            writer.EndObject();
        }
        writer.EndArray();
        writer.EndObject();

        std::fclose(file);
    }

    const std::string& Leaderboard::GetUsername() const
    {
        return mUsername;
    }

    void Leaderboard::SetUsername(const std::string& name)
    {
        mUsername = name;
        if (static_cast<int>(mUsername.size()) > kMaxUsernameLen)
        {
            mUsername.resize(kMaxUsernameLen);
        }
    }

    float Leaderboard::GetVolume() const
    {
        return mVolume;
    }

    void Leaderboard::SetVolume(float volume)
    {
        mVolume = std::clamp(volume, 0.0f, 1.0f);
    }

    bool Leaderboard::GetRender() const
    {
        return mRender;
    }

    void Leaderboard::SetRender(bool render)
    {
        mRender = render;
    }

    int Leaderboard::GetPB() const
    {
        return mPB;
    }

    void Leaderboard::Post(const std::string& name, int score)
    {
        mScores.push_back({name, score});

        // Sort DESC by score. stable_sort keeps insertion order for ties so the
        // newest run does not leapfrog an equal-scoring earlier entry.
        std::stable_sort(
            mScores.begin(), mScores.end(), [](const Entry& a, const Entry& b) { return a.score > b.score; });

        // Trim to exactly the top-10.
        if (static_cast<int>(mScores.size()) > kMaxScores)
        {
            mScores.resize(kMaxScores);
        }

        // PB = max(pb, best submitted).
        if (score > mPB)
        {
            mPB = score;
        }
    }

    const std::vector<Leaderboard::Entry>& Leaderboard::Entries() const
    {
        return mScores;
    }

    int Leaderboard::PlayerRowIndex(const std::string& name, int score) const
    {
        for (int i = 0; i < static_cast<int>(mScores.size()); ++i)
        {
            if (mScores[i].name == name && mScores[i].score == score)
            {
                return i;
            }
        }
        return -1;
    }

    bool LeaderboardSelfTest(std::string& out)
    {
        std::ostringstream log;
        bool pass = true;
        auto check = [&](const std::string& label, bool ok) {
            log << (ok ? "PASS" : "FAIL") << " : " << label << "\n";
            pass = pass && ok;
        };

        const std::string tempPath = "criticalcore_save_selftest.json";
        std::remove(tempPath.c_str());

        // --- Run A: submit three runs, Save, reload, assert order + PB ---
        {
            Leaderboard board(tempPath);
            board.Load(); // missing temp file -> empty, no crash
            check("fresh load is empty", board.Entries().empty());
            check("fresh PB is 0", board.GetPB() == 0);

            board.Post("AAA", 100);
            board.Post("BBB", 300);
            board.Post("CCC", 200);
            board.Save();
        }

        {
            Leaderboard board(tempPath);
            board.Load();
            const auto& entries = board.Entries();
            check("reload has 3 entries", entries.size() == 3);
            if (entries.size() == 3)
            {
                check("order[0] == BBB(300)", entries[0].name == "BBB" && entries[0].score == 300);
                check("order[1] == CCC(200)", entries[1].name == "CCC" && entries[1].score == 200);
                check("order[2] == AAA(100)", entries[2].name == "AAA" && entries[2].score == 100);
            }
            else
            {
                check("order assertions skipped (wrong count)", false);
            }
            check("PB == 300", board.GetPB() == 300);
            check("PlayerRowIndex(CCC,200) == 1", board.PlayerRowIndex("CCC", 200) == 1);
            check("PlayerRowIndex(missing) == -1", board.PlayerRowIndex("ZZZ", 999) == -1);
        }

        // --- Corrupt the file, then Load asserts empty + no crash ---
        {
            FILE* file = OpenFile(tempPath.c_str(), "wb");
            if (file != nullptr)
            {
                const char* garbage = "{ this is not valid json :: ]]";
                std::fwrite(garbage, 1, std::strlen(garbage), file);
                std::fclose(file);
            }

            Leaderboard board(tempPath);
            board.Load(); // must not throw/crash
            check("corrupt file -> empty entries", board.Entries().empty());
            check("corrupt file -> PB 0", board.GetPB() == 0);
            check("corrupt file -> username empty", board.GetUsername().empty());
        }

        // --- Settings persist across Save/Load ---
        {
            Leaderboard board(tempPath);
            board.Load(); // reads corrupt -> empty
            board.SetUsername("Player1");
            board.SetVolume(0.42f);
            board.SetRender(false);
            board.Post("DEF", 555);
            board.Save();

            Leaderboard reloaded(tempPath);
            reloaded.Load();
            check("settings: username persists", reloaded.GetUsername() == "Player1");
            check("settings: volume persists", std::abs(reloaded.GetVolume() - 0.42f) < 1e-4f);
            check("settings: render persists", reloaded.GetRender() == false);
            check("settings: PB persists", reloaded.GetPB() == 555);
        }

        // --- Username clamp to <=10 chars ---
        {
            Leaderboard board(tempPath);
            board.SetUsername("ThisNameIsWayTooLong");
            check("username clamped to 10 chars", board.GetUsername().size() == 10);
        }

        // --- Trim to exactly top-10 ---
        {
            Leaderboard board(tempPath);
            for (int i = 0; i < 15; ++i)
            {
                board.Post("P" + std::to_string(i), i * 10);
            }
            check("trimmed to 10 entries", board.Entries().size() == 10);
            check("top entry is highest (140)", board.Entries().front().score == 140);
            check("last kept entry is 50", board.Entries().back().score == 50);
        }

        std::remove(tempPath.c_str());

        log << (pass ? "RESULT PASS" : "RESULT FAIL") << "\n";
        out = log.str();
        return pass;
    }
}
