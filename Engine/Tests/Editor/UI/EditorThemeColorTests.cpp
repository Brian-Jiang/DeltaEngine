#include "Editor/Style/EditorTheme.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

namespace
{
constexpr float Inv255 = 1.f / 255.f;
}

TEST(EditorThemeColorTests, SixDigitOpaqueRed)
{
    ImVec4 v = EditorTheme::ColorFromHex(0xFF0000);
    EXPECT_NEAR(v.x, 1.f, 1e-6f);
    EXPECT_NEAR(v.y, 0.f, 1e-6f);
    EXPECT_NEAR(v.z, 0.f, 1e-6f);
    EXPECT_NEAR(v.w, 1.f, 1e-6f);
}

TEST(EditorThemeColorTests, EightDigitUsesAlphaChannel)
{
    ImVec4 v = EditorTheme::ColorFromHex(0xFF000080);
    EXPECT_NEAR(v.x, 1.f, 1e-6f);
    EXPECT_NEAR(v.y, 0.f, 1e-6f);
    EXPECT_NEAR(v.z, 0.f, 1e-6f);
    EXPECT_NEAR(v.w, 128.f * Inv255, 1e-5f);
}

TEST(EditorThemeColorTests, AlphaOverrideWins)
{
    ImVec4 v = EditorTheme::ColorFromHex(0xFF0000FF, 0.25f);
    EXPECT_NEAR(v.x, 1.f, 1e-6f);
    EXPECT_NEAR(v.w, 0.25f, 1e-6f);
}

TEST(EditorThemeColorTests, BlackAndFullWhite)
{
    ImVec4 b = EditorTheme::ColorFromHex(0x000000);
    EXPECT_NEAR(b.x, 0.f, 1e-6f);
    EXPECT_NEAR(b.w, 1.f, 1e-6f);
    ImVec4 w = EditorTheme::ColorFromHex(0xFFFFFF);
    EXPECT_NEAR(w.x, 1.f, 1e-6f);
    EXPECT_NEAR(w.y, 1.f, 1e-6f);
    EXPECT_NEAR(w.z, 1.f, 1e-6f);
}

TEST(EditorThemeColorTests, LeadingZeroRgbStillSixDigit)
{
    ImVec4 v = EditorTheme::ColorFromHex(0x00112233u);
    EXPECT_NEAR(v.x, 0x11 * Inv255, 1e-5f);
    EXPECT_NEAR(v.y, 0x22 * Inv255, 1e-5f);
    EXPECT_NEAR(v.z, 0x33 * Inv255, 1e-5f);
    EXPECT_NEAR(v.w, 1.f, 1e-6f);
}
