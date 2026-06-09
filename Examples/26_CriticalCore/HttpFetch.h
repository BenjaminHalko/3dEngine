#pragma once

#include <string>

namespace Engine::CriticalCore
{
    // Synchronously fetch the body of a URL via the system `curl` binary.
    // Returns true on success (HTTP 2xx + non-empty body); false on any error
    // (curl missing, timeout, non-2xx response, empty body). On failure `bodyOut`
    // is empty.
    //
    // Implementation uses popen() so it runs in-process with no new dependency -
    // macOS, Linux, and modern Windows (>=10/1803) all ship curl. The URL is
    // single-quoted into the shell command, so DO NOT pass a URL containing a
    // single quote (this is fine for the Firebase REST endpoints in use here).
    bool HttpGet(const std::string& url, std::string& bodyOut, int timeoutSec = 10);
}
