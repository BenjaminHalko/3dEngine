add_library(assimp SHARED IMPORTED GLOBAL)
target_include_directories(assimp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(WIN32)
    set_target_properties(assimp PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/assimp.lib
    )
elseif(APPLE)
    set_target_properties(assimp PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/libassimp.dylib
    )
else()
    set_target_properties(assimp PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/libassimp.so
    )
endif()
