cmake_minimum_required(VERSION 3.16)
project(3dEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(external)
add_subdirectory(engine)
add_subdirectory(framework)
add_subdirectory(examples)
