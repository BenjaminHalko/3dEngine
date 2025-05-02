#pragma once

#include "timeutil.h"

using namespace Engine::Core;

#ifdef _DEBUG
#define LOG(format, ...);\
    do {\
        char _buffer[256];\
        auto _res = snprintf(_buffer, sizeof(_buffer), "{%.3f}: "##format##"\n", TimeUtil::GetTime(), __VA_ARGS__);\
        printf_s(_buffer);\
    } while (0)
#define ASSERT(condition, format, ...);\
    do {\
        if (!(condition)) {\
            LOG("ASSERTION FAILED: %s(%d)\n"##format##, __FILE__, __LINE__, __VA_ARGS__);\
            DebugBreak();\
        }\
    } while (0)
#else
#define LOG(format, ...)
#define ASSERT(condition, format, ...) do{ (void)sizeof(condition); } while(0)
#endif
