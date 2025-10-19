cmake_minimum_required(VERSION 3.16)
project(3dEngine LANGUAGES CXX)

set(DIRECTX_FXC_TOOL "C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe" CACHE FILEPATH "Path to fxc.exe")

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(external)
add_subdirectory(engine)
add_subdirectory(framework)
add_subdirectory(examples)
