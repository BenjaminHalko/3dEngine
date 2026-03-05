project(Audio)
include(${CMAKE_SOURCE_DIR}/Cmake/framework_lib.cmake)

target_compile_library(Audio)

target_include_directories(Audio PUBLIC
    ${CMAKE_SOURCE_DIR}/External
)

target_link_libraries(Audio
    PRIVATE Core
)

if(APPLE)
    target_link_libraries(Audio PRIVATE
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework CoreFoundation"
    )
elseif(UNIX AND NOT APPLE)
    target_link_libraries(Audio PRIVATE
        pthread m dl
    )
endif()
