#include <gtest/gtest.h>

// ── Smoke test ────────────────────────────────────────────────────────────────
// This file validates that GoogleTest is correctly wired up and that the
// test binary can be built and run without a display (headless).
// Add real game-logic tests alongside this file as the project grows.

TEST(Smoke, BasicArithmetic)
{
    EXPECT_EQ(1 + 1, 2);
    EXPECT_NE(1 + 1, 3);
}

TEST(Smoke, CppStandardIsAtLeast20)
{
    // __cplusplus is 202002L for C++20
    EXPECT_GE(__cplusplus, 202002L);
}
