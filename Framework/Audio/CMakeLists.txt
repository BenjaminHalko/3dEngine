project(Audio)
include(${CMAKE_SOURCE_DIR}/Cmake/framework_lib.cmake)

target_compile_library(Audio)

# Vendored single-file stb_vorbis decoder (compiled once as its own C TU to
# avoid ODR issues). It is C, not C++, so the C++ precompiled header is skipped.
target_sources(Audio PRIVATE
    ${CMAKE_SOURCE_DIR}/External/stb/stb_vorbis.c
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/External/stb/stb_vorbis.c
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON
)

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
