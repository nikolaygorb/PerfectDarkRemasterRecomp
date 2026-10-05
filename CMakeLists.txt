# perfectdarkremasterrecomp - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(perfectdarkremasterrecomp LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Fix git symlinks on Windows (libmspack in SDK uses symlinks)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/fix_symlinks.cmake)

# CPU optimization presets
#include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_optimizations.cmake)

# Build ReXGlue SDK from source (git submodule)
set(REXSDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/rexglue-sdk" CACHE PATH "Path to rexglue-sdk source tree" FORCE)

# Enable both graphics backends
if(WIN32)
    set(REXGLUE_USE_D3D12 ON CACHE BOOL "Enable D3D12 graphics backend")
endif()
set(REXGLUE_USE_VULKAN ON CACHE BOOL "Enable Vulkan graphics backend")

include(generated/rexglue.cmake)

# Sources
set(PERFECTDARKREMASTERRECOMP_SOURCES
    src/main.cpp
    src/game_cvars.cpp
    src/thread_sampler.cpp
    src/audio/event_queue.cpp
    src/render/gpu_wait_hook.cpp
    src/platform/live_prompt.cpp
    src/mod/third_person/third_person.cpp
    src/mod/third_person/body.cpp
    src/mod/third_person/body_weapons.cpp
    src/mod/third_person/body_reload.cpp
    src/mod/third_person/camera.cpp
    src/mod/third_person/hud.cpp
)

# Windows resources (icon)
if(WIN32)
    list(APPEND PERFECTDARKREMASTERRECOMP_SOURCES
        src/perfectdarkremasterrecomp.rc
    )
endif()

if(WIN32)
    add_executable(perfectdarkremasterrecomp WIN32 ${PERFECTDARKREMASTERRECOMP_SOURCES})
else()
    add_executable(perfectdarkremasterrecomp ${PERFECTDARKREMASTERRECOMP_SOURCES})
endif()

rexglue_setup_target(perfectdarkremasterrecomp GPU_PLUGINS xenos)

# Link against rexui for UI support (imgui, etc.)
target_link_libraries(perfectdarkremasterrecomp PRIVATE rex::ui)

# SDL looks for this file (name set by settings/mapping.toml's
# hid_mappings_file) next to the exe at startup to fill in gamepads it
# doesn't already recognize. Sourced from
# https://github.com/mdqinc/SDL_GameControllerDB, see settings/README.md.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/settings/gamecontrollerdb.txt")
    add_custom_command(TARGET perfectdarkremasterrecomp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/settings/gamecontrollerdb.txt"
                "$<TARGET_FILE_DIR:perfectdarkremasterrecomp>/gamecontrollerdb.txt"
        COMMENT "Copying gamecontrollerdb.txt next to perfectdarkremasterrecomp.exe")
endif()
