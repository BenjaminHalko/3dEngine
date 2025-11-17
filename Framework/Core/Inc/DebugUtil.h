#pragma once

#include "TimeUtil.h"

using namespace Engine;
using namespace Engine::Core;

#ifdef _WIN32
    #include <windows.h>
    #define PLATFORM_BREAK() DebugBreak()
#else
    #include <signal.h>
    #define PLATFORM_BREAK() raise(SIGTRAP)
#endif

#if defined(_DEBUG)
#define LOG(format, ...)                                                                           \
    ;                                                                                              \
    do                                                                                             \
    {                                                                                              \
        char _buffer[256];                                                                         \
        int _res = snprintf(_buffer,                                                               \
                            std::size(_buffer),                                                    \
                            "{%.3f}: " format "\n",                                                \
                            TimeUtil::GetTime(),                                                   \
                            __VA_ARGS__);                                                          \
        std::cout << _buffer;                                                                      \
    } while (false)

#define ASSERT(condition, format, ...)                                                             \
    do                                                                                             \
    {                                                                                              \
        if (!(condition))                                                                          \
        {                                                                                          \
            LOG("ASSERT! %s(%d)\n" format, __FILE__, __LINE__, __VA_ARGS__);                       \
            PLATFORM_BREAK();                                                                          \
        }                                                                                          \
    } while (false)
#else
#define LOG(format, ...)
#define ASSERT(condition, format, ...)                                                             \
    do                                                                                             \
    {                                                                                              \
        (void) sizeof(condition);                                                                  \
    } while (false)
#endif
