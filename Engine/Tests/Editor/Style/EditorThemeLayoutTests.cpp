#include "Editor/Style/EditorTheme.h"

#include "Editor/Panels/EditorChromeTestImGui.h"

#include <gtest/gtest.h>

using DeltaEngine::EditorTheme;

class EditorThemeLayoutFixture : public ::testing::Test
{
protected:
    void SetUp() override
    {
        DeltaEngine::EditorChrome_CreateTestImGuiContext(ImVec2(1280.f, 720.f));
    }

    void TearDown() override { DeltaEngine::EditorChrome_DestroyTestImGuiContext(); }
};

TEST_F(EditorThemeLayoutFixture, EditorTheme_RowH_WithDefaultStyle_IsPositive)
{
    EXPECT_GT(EditorTheme::RowH(), 0.f);
}

TEST_F(EditorThemeLayoutFixture, EditorTheme_HdrTbSt_WithDefaultStyle_OrderedSanity)
{
    EXPECT_GT(EditorTheme::HdrH() + EditorTheme::TbH(), EditorTheme::StH());
}
