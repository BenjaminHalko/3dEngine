# Find DirectX Shader Compiler (fxc.exe)
find_program(DIRECTX_FXC_TOOL 
    NAMES fxc.exe
    PATHS
        "C:/Program Files (x86)/Windows Kits/10/bin/10.0.22621.0/x64"
        "C:/Program Files (x86)/Windows Kits/10/bin/10.0.22621.0/x86"
        "C:/Program Files (x86)/Windows Kits/10/bin/10.0.19041.0/x64"  
        "C:/Program Files (x86)/Windows Kits/10/bin/10.0.19041.0/x86"
        "C:/Program Files (x86)/Windows Kits/10/bin/10.0.18362.0/x64"
        "C:/Program Files (x86)/Windows Kits/10/bin/10.0.18362.0/x86"
        "C:/Program Files (x86)/Windows Kits/10/bin/x64"
        "C:/Program Files (x86)/Windows Kits/10/bin/x86"
        "C:/Program Files (x86)/Microsoft Visual Studio/2022/Professional/Common7/IDE/Extensions/Microsoft/VsGraphics/bin"
        "C:/Program Files (x86)/Microsoft Visual Studio/2022/Community/Common7/IDE/Extensions/Microsoft/VsGraphics/bin"
        "C:/Program Files/Microsoft Visual Studio/2022/Professional/Common7/IDE/Extensions/Microsoft/VsGraphics/bin"
        "C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/Extensions/Microsoft/VsGraphics/bin"
    DOC "DirectX Shader Compiler (fxc.exe)"
)

if(DIRECTX_FXC_TOOL)
    message(STATUS "Found DirectX Shader Compiler: ${DIRECTX_FXC_TOOL}")
else()
    message(WARNING "DirectX Shader Compiler (fxc.exe) not found. Please install Windows SDK or Visual Studio with DirectX components.")
    message(STATUS "You may need to add fxc.exe location to your system PATH or install:")
    message(STATUS "  - Windows 10/11 SDK")
    message(STATUS "  - Visual Studio 2022 with C++ Desktop Development workload")
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Suppress external library warnings
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wno-nontrivial-memcall)
    add_compile_options(-Wno-nonportable-include-path)
endif()
