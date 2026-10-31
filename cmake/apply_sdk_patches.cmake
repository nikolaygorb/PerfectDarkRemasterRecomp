# apply_sdk_patches.cmake
#
# Applies the project's patches in patches/rexglue-sdk/ to the ReXGlue SDK
# submodule at configure time. Each patch is applied once: a patch that already
# reverses cleanly is skipped, so re-configuring is harmless. Each *.patch has
# a *.md next to it explaining why it exists and when it can be dropped.

# The patches only change behavior on macOS (MoltenVK).
if(NOT APPLE)
    return()
endif()

find_package(Git REQUIRED)

set(REXSDK_PATCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/patches/rexglue-sdk")
file(GLOB REXSDK_PATCHES "${REXSDK_PATCH_DIR}/*.patch")
list(SORT REXSDK_PATCHES)

foreach(patch ${REXSDK_PATCHES})
    get_filename_component(patch_name "${patch}" NAME)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" apply --reverse --check "${patch}"
        WORKING_DIRECTORY "${REXSDK_DIR}"
        RESULT_VARIABLE already_applied
        OUTPUT_QUIET ERROR_QUIET)
    if(already_applied EQUAL 0)
        continue()
    endif()
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" apply "${patch}"
        WORKING_DIRECTORY "${REXSDK_DIR}"
        RESULT_VARIABLE apply_result
        ERROR_VARIABLE apply_error)
    if(NOT apply_result EQUAL 0)
        message(FATAL_ERROR "Cannot apply ${patch_name} to ${REXSDK_DIR}:\n${apply_error}")
    endif()
    message(STATUS "Applied SDK patch: ${patch_name}")
endforeach()
