# CMake module for compiling HLSL shaders to DXBC (.cso files)
# Works on both Windows (native fxc.exe) and macOS (Wine + fxc.exe)

# Find the shader compiler - use bundled fxc.exe on both platforms
set(FXC_EXE_PATH "${CMAKE_SOURCE_DIR}/Tools/fxc/fxc.exe")

if(WIN32)
    # On Windows, use fxc.exe directly
    if(EXISTS "${FXC_EXE_PATH}")
        set(FXC_EXECUTABLE "${FXC_EXE_PATH}")
        set(FXC_USE_WINE OFF)
    endif()
elseif(APPLE)
    # On macOS, use Wine + fxc.exe
    find_program(WINE_EXECUTABLE wine HINTS /opt/homebrew/bin /usr/local/bin)
    if(EXISTS "${FXC_EXE_PATH}" AND WINE_EXECUTABLE)
        set(FXC_EXECUTABLE "${WINE_EXECUTABLE}")
        set(FXC_USE_WINE ON)
    endif()
endif()

# Function to convert Unix path to Wine Z: path
function(to_wine_path UNIX_PATH OUT_VAR)
    string(REPLACE "/" "\\\\" WINE_PATH "${UNIX_PATH}")
    set(${OUT_VAR} "Z:${WINE_PATH}" PARENT_SCOPE)
endfunction()

# Function to compile a single shader
# Usage: compile_shader(SOURCE_FILE ENTRY_POINT PROFILE OUTPUT_FILE)
function(compile_shader SOURCE_FILE ENTRY_POINT PROFILE OUTPUT_FILE)
    if(NOT FXC_EXECUTABLE)
        message(WARNING "FXC compiler not found, skipping shader compilation")
        return()
    endif()

    get_filename_component(OUTPUT_DIR "${OUTPUT_FILE}" DIRECTORY)
    file(MAKE_DIRECTORY "${OUTPUT_DIR}")

    if(WIN32)
        add_custom_command(
            OUTPUT "${OUTPUT_FILE}"
            COMMAND "${FXC_EXE_PATH}"
                /T ${PROFILE}
                /E ${ENTRY_POINT}
                /Fo "${OUTPUT_FILE}"
                "${SOURCE_FILE}"
            DEPENDS "${SOURCE_FILE}"
            COMMENT "Compiling shader ${ENTRY_POINT} from ${SOURCE_FILE}"
            VERBATIM
        )
    elseif(APPLE)
        # Convert paths for Wine
        to_wine_path("${SOURCE_FILE}" WINE_SOURCE)
        to_wine_path("${OUTPUT_FILE}" WINE_OUTPUT)
        to_wine_path("${FXC_EXE_PATH}" WINE_FXC)

        add_custom_command(
            OUTPUT "${OUTPUT_FILE}"
            COMMAND ${CMAKE_COMMAND} -E env "DXMT_LOG_LEVEL=none"
                ${WINE_EXECUTABLE} "${WINE_FXC}"
                /T ${PROFILE}
                /E ${ENTRY_POINT}
                /Fo "${WINE_OUTPUT}"
                "${WINE_SOURCE}"
            DEPENDS "${SOURCE_FILE}"
            COMMENT "Compiling shader ${ENTRY_POINT} from ${SOURCE_FILE} (via Wine)"
            VERBATIM
        )
    endif()
endfunction()

# Function to compile all entry points from a shader file
# Standard convention: VS for vertex shader, PS for pixel shader
# Usage: compile_shader_file(SOURCE_FILE OUTPUT_DIR [ENTRIES entry1 entry2...])
function(compile_shader_file SOURCE_FILE OUTPUT_DIR)
    cmake_parse_arguments(PARSE_ARGV 2 ARG "" "" "ENTRIES")

    get_filename_component(SHADER_NAME "${SOURCE_FILE}" NAME_WE)

    # Default entries if not specified
    if(NOT ARG_ENTRIES)
        set(ARG_ENTRIES VS PS)
    endif()

    set(COMPILED_SHADERS "")

    foreach(ENTRY ${ARG_ENTRIES})
        # Determine profile based on entry point name
        if(ENTRY MATCHES "^VS")
            set(PROFILE "vs_5_0")
        elseif(ENTRY MATCHES "^PS")
            set(PROFILE "ps_5_0")
        elseif(ENTRY MATCHES "^GS")
            set(PROFILE "gs_5_0")
        elseif(ENTRY MATCHES "^HS")
            set(PROFILE "hs_5_0")
        elseif(ENTRY MATCHES "^DS")
            set(PROFILE "ds_5_0")
        elseif(ENTRY MATCHES "^CS")
            set(PROFILE "cs_5_0")
        else()
            message(WARNING "Unknown shader entry point type: ${ENTRY}")
            continue()
        endif()

        set(OUTPUT_FILE "${OUTPUT_DIR}/${SHADER_NAME}_${ENTRY}.cso")
        compile_shader("${SOURCE_FILE}" "${ENTRY}" "${PROFILE}" "${OUTPUT_FILE}")
        list(APPEND COMPILED_SHADERS "${OUTPUT_FILE}")
    endforeach()

    # Return list of compiled shaders
    set(${SHADER_NAME}_COMPILED_SHADERS "${COMPILED_SHADERS}" PARENT_SCOPE)
endfunction()

# Function to compile all shaders in a directory
# Usage: compile_all_shaders(SOURCE_DIR OUTPUT_DIR TARGET_NAME)
function(compile_all_shaders SOURCE_DIR OUTPUT_DIR TARGET_NAME)
    file(GLOB SHADER_FILES "${SOURCE_DIR}/*.fx" "${SOURCE_DIR}/*.hlsl")

    set(ALL_COMPILED_SHADERS "")

    foreach(SHADER_FILE ${SHADER_FILES})
        compile_shader_file("${SHADER_FILE}" "${OUTPUT_DIR}")
        get_filename_component(SHADER_NAME "${SHADER_FILE}" NAME_WE)
        list(APPEND ALL_COMPILED_SHADERS ${${SHADER_NAME}_COMPILED_SHADERS})
    endforeach()

    # Create a custom target for shader compilation
    if(ALL_COMPILED_SHADERS)
        add_custom_target(${TARGET_NAME}_shaders DEPENDS ${ALL_COMPILED_SHADERS})
    endif()
endfunction()
