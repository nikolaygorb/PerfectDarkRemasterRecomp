# cpu_presets.cmake
#
# CPU optimization presets. Pick one via REX_CPU_PRESET.
#
# Presets:
#   native     - aggressive, target the current CPU (-march=native)
#   avx512     - AVX-512 (Skylake-X / Ice Lake / Ice Lake-S / newer)
#   avx2       - AVX2 / FMA / BMI2 (Haswell / Broadwell / newer)
#   generic    - x86-64 baseline + SSSE3 (portable)
#   none       - no extra flags

if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64|x86_64")
    return()
endif()

set(REX_CPU_PRESET "native" CACHE STRING
    "CPU optimization preset: native, avx512, avx2, generic, none")
set_property(CACHE REX_CPU_PRESET PROPERTY STRINGS native avx512 avx2 generic none)

if(REX_CPU_PRESET STREQUAL "native")
    add_compile_options(-march=native)
elseif(REX_CPU_PRESET STREQUAL "avx512")
    add_compile_options(-march=avx512)
elseif(REX_CPU_PRESET STREQUAL "avx2")
    add_compile_options(-march=avx2)
elseif(REX_CPU_PRESET STREQUAL "generic")
    add_compile_options(-march=x86-64)
elseif(REX_CPU_PRESET STREQUAL "none")
    # No extra flags.
endif()

message(STATUS "CPU preset: ${REX_CPU_PRESET}")
