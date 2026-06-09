#include "HttpFetch.h"

#include <array>
#include <cstdio>
#include <sstream>

namespace Engine::CriticalCore
{
    namespace
    {
#if defined(_WIN32)
        FILE* OpenPipe(const char* cmd) { return _popen(cmd, "r"); }
        int   ClosePipe(FILE* p)        { return _pclose(p); }
#else
        FILE* OpenPipe(const char* cmd) { return popen(cmd, "r"); }
        int   ClosePipe(FILE* p)        { return pclose(p); }
#endif
    }

    bool HttpGet(const std::string& url, std::string& bodyOut, int timeoutSec)
    {
        bodyOut.clear();

        // curl flags:
        //   -s        silent (no progress meter)
        //   -S        show errors on stderr (so we don't fail "quietly")
        //   --fail    pclose returns non-zero on HTTP 4xx/5xx
        //   --max-time guards against indefinite hangs
        // The URL is single-quoted; callers must not pass URLs containing '.
        std::ostringstream cmd;
        cmd << "curl -sS --fail --max-time " << timeoutSec << " '" << url << "'";

        FILE* pipe = OpenPipe(cmd.str().c_str());
        if (pipe == nullptr)
        {
            return false;
        }

        std::array<char, 8192> buffer{};
        std::ostringstream out;
        while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
        {
            out << buffer.data();
        }

        const int rc = ClosePipe(pipe);
        if (rc != 0)
        {
            return false;
        }

        bodyOut = out.str();
        return !bodyOut.empty();
    }
}
