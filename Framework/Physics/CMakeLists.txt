project(Physics)
include(${CMAKE_SOURCE_DIR}/Cmake/framework_lib.cmake)

target_compile_library(Physics)

target_include_directories(Physics PUBLIC
    ${CMAKE_SOURCE_DIR}/External/bullet3/src
)

target_link_libraries(Physics
    PUBLIC BulletDynamics BulletCollision LinearMath BulletSoftBody
    PRIVATE Core Math Graphics
)
