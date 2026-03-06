# ── Game sources ─────────────────────────────────────────────────────────────
# Add all game logic source files to defender_lib (defined in the root CMakeLists).
target_sources(defender_lib
    PRIVATE
        # Add your .cpp files here as the project grows, e.g.:
        # Game.cpp
        # Tower.cpp
        # Enemy.cpp
)

# ── Executable ───────────────────────────────────────────────────────────────
add_executable(defender main.cpp)
target_link_libraries(defender PRIVATE defender_lib)

# Copy SFML DLLs next to the executable on Windows (no-op on Linux/macOS)
if(WIN32)
    add_custom_command(TARGET defender POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:defender>
            $<TARGET_FILE_DIR:defender>
        COMMAND_EXPAND_LISTS
    )
endif()
