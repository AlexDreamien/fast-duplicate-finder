cmake_minimum_required(VERSION 3.20)
project(dupfinder LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Warnings
if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Threads
find_package(Threads REQUIRED)

# Vendored xxHash (header-only, fully inline)
add_library(xxhash INTERFACE)
target_include_directories(xxhash INTERFACE third_party/xxhash)
target_compile_definitions(xxhash INTERFACE XXH_INLINE_ALL)

# Core library
add_library(dupfinder_core
    src/scan.cpp
    src/hashing.cpp
    src/finder_v0.cpp
    src/finder_v1.cpp
    src/finder_v2.cpp
    src/finder_v3.cpp
    src/format.cpp
)
target_include_directories(dupfinder_core PUBLIC include)
target_link_libraries(dupfinder_core PUBLIC xxhash Threads::Threads)

# CLI executable
add_executable(dupfinder src/main.cpp)
target_link_libraries(dupfinder PRIVATE dupfinder_core)

# Tests
option(DUPFINDER_BUILD_TESTS "Build unit tests" ON)
if(DUPFINDER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
