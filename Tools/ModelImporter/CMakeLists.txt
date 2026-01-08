project(ModelImporter)

include_directories(${CMAKE_SOURCE_DIR}/Framework ${CMAKE_SOURCE_DIR}/Engine ${CMAKE_SOURCE_DIR}/External)

add_executable(ModelImporter main.cpp)

target_link_libraries(ModelImporter
    Engine
    assimp
)
