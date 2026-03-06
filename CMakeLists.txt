cmake_minimum_required(VERSION 3.21)
project(defender VERSION 0.1.0 LANGUAGES CXX)

# ── C++20 ───────────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Global warnings ──────────────────────────────────────────────────────────
add_compile_options(-Wall -Wextra -Wpedantic)

# ── Dependencies via FetchContent ────────────────────────────────────────────
include(FetchContent)

# SFML 2.6.x
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        2.6.x
    GIT_SHALLOW    TRUE
)
set(SFML_BUILD_EXAMPLES   OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC        OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_TEST_SUITE OFF CACHE BOOL "" FORCE)
# Audio is used — Network is not needed
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SFML)

# GoogleTest
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
    GIT_SHALLOW    TRUE
)
# Prevent GoogleTest from overriding our compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ── Shared game-logic library (no main, no window) ───────────────────────────
# All game logic goes in src/ and is compiled into defender_lib.
# This lets tests link against it without pulling in SFML display dependencies.
add_library(defender_lib STATIC
    src/defender_lib.cpp
)
target_include_directories(defender_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)
target_link_libraries(defender_lib
    PUBLIC
        sfml-graphics
        sfml-window
        sfml-system
        sfml-audio
)

# ── Subdirectories ───────────────────────────────────────────────────────────
add_subdirectory(src)

# ── Tests ────────────────────────────────────────────────────────────────────
enable_testing()
include(CTest)
add_subdirectory(tests)
