# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

# DirectX11 compatibility
if (NOT WIN32)
    add_subdirectory(vkd3d)

    if (APPLE)
        add_subdirectory(dxmt)
    else()
        add_subdirectory(dxvk)
    endif()
endif()

# Assimp
add_subdirectory(assimp)

# ImGui
add_subdirectory(ImGui)
