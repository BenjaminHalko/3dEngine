# MinGW-w64 Cross-Compilation Toolchain (macOS/Linux → Windows)

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Find MinGW-w64 compiler (works on macOS and Linux)
find_program(CMAKE_C_COMPILER NAMES x86_64-w64-mingw32-gcc PATHS /opt/homebrew/bin /usr/local/bin /usr/bin)
find_program(CMAKE_CXX_COMPILER NAMES x86_64-w64-mingw32-g++ PATHS /opt/homebrew/bin /usr/local/bin /usr/bin)
find_program(CMAKE_RC_COMPILER NAMES x86_64-w64-mingw32-windres PATHS /opt/homebrew/bin /usr/local/bin /usr/bin)

if(NOT CMAKE_C_COMPILER OR NOT CMAKE_CXX_COMPILER)
    message(FATAL_ERROR "MinGW-w64 not found!\n"
        "macOS: brew install mingw-w64\n"
        "Linux: sudo apt install mingw-w64\n"
        "Then reconfigure CMake.")
endif()

# Adjust find behavior for cross-compilation
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Target Windows
set(WIN32 TRUE)
set(MINGW TRUE)

# Static link MinGW runtime libraries to avoid DLL dependencies in Wine
set(CMAKE_CXX_FLAGS_INIT "-static-libgcc -static-libstdc++ -static")
set(CMAKE_C_FLAGS_INIT "-static-libgcc -static")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static-libgcc -static-libstdc++ -static -lpthread")
