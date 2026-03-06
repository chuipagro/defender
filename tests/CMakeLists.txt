include(GoogleTest)

# ── Test executable ───────────────────────────────────────────────────────────
add_executable(defender_tests
    test_smoke.cpp
)

target_link_libraries(defender_tests
    PRIVATE
        defender_lib
        GTest::gtest_main
)

# Automatically register discovered tests with CTest
gtest_discover_tests(defender_tests)
