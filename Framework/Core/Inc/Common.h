#pragma once

// GLFW for all platforms
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// Std Headers
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

// Cross-platform compatibility for Windows-specific secure functions
#ifndef _WIN32
    #include <cstdarg>
    #include <cwchar>
    #include <cerrno>

    using errno_t = int;

    // fopen_s replacement
    inline errno_t fopen_s(FILE** pFile, const char* filename, const char* mode) {
        if (!pFile) return EINVAL;
        *pFile = fopen(filename, mode);
        return (*pFile) ? 0 : errno;
    }

    // fprintf_s is equivalent to fprintf on POSIX
    #define fprintf_s fprintf

    // fscanf_s is equivalent to fscanf on POSIX (note: no buffer size checking)
    #define fscanf_s fscanf

    // swprintf_s replacement
    inline int swprintf_s(wchar_t* buffer, size_t sizeOfBuffer, const wchar_t* format, ...) {
        va_list args;
        va_start(args, format);
        int result = vswprintf(buffer, sizeOfBuffer, format, args);
        va_end(args);
        return result;
    }
#endif
