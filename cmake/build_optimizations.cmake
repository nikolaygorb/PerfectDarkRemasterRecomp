# build_optimizations.cmake

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    # Release
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
        set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto=thin -march=x86-64-v2 -DNDEBUG")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
        set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto=thin -DNDEBUG")
    else()
        set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto=thin -DNDEBUG")
    endif()

    # RelWithDebInfo
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")

    # Debug
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
endif()