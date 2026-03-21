#include "Editor/EditorWindows/EditorWindow_ViewportPresets.h"

#include <gtest/gtest.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 6326)
#endif

using namespace DeltaEngine;

TEST(ViewportPresetTests, PresetLabels)
{
    EXPECT_STREQ(GetResolutionPresetLabel(ViewportResolution::FreeAspect), "Free Aspect");
    EXPECT_STREQ(GetResolutionPresetLabel(ViewportResolution::Resolution_1280x720), "1280 x 720");
    EXPECT_STREQ(GetResolutionPresetLabel(ViewportResolution::Resolution_1920x1080), "1920 x 1080");
    EXPECT_STREQ(GetResolutionPresetLabel(ViewportResolution::Resolution_3840x2160), "3840 x 2160 (4K)");
    EXPECT_STREQ(GetResolutionPresetLabel(ViewportResolution::Count), "Unknown");
}

TEST(ViewportPresetTests, PresetSizes)
{
    int w = -1;
    int h = -1;
    GetResolutionPresetSize(ViewportResolution::FreeAspect, w, h);
    EXPECT_EQ(w, 0);
    EXPECT_EQ(h, 0);

    GetResolutionPresetSize(ViewportResolution::Count, w, h);
    EXPECT_EQ(w, 0);
    EXPECT_EQ(h, 0);

    GetResolutionPresetSize(ViewportResolution::Resolution_1280x720, w, h);
    EXPECT_EQ(w, 1280);
    EXPECT_EQ(h, 720);

    GetResolutionPresetSize(ViewportResolution::Resolution_1920x1080, w, h);
    EXPECT_EQ(w, 1920);
    EXPECT_EQ(h, 1080);

    GetResolutionPresetSize(ViewportResolution::Resolution_3840x2160, w, h);
    EXPECT_EQ(w, 3840);
    EXPECT_EQ(h, 2160);
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
