# perfectdarkremasterrecomp - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(perfectdarkremasterrecomp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
set(PERFECTDARKREMASTERRECOMP_SOURCES
    src/main.cpp
)

if(WIN32)
    add_executable(perfectdarkremasterrecomp WIN32 ${PERFECTDARKREMASTERRECOMP_SOURCES})
else()
    add_executable(perfectdarkremasterrecomp ${PERFECTDARKREMASTERRECOMP_SOURCES})
endif()

rexglue_setup_target(perfectdarkremasterrecomp GPU_PLUGINS xenos)

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
