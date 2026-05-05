project(Engine)
include(${CMAKE_SOURCE_DIR}/Cmake/engine_lib.cmake)

target_compile_library(Engine)
target_link_libraries(Engine PUBLIC Core Graphics Input Math Physics Audio rapidjson)
