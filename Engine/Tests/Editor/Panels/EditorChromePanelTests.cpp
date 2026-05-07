#include "Editor/Panels/EditorChromeTestImGui.h"

#include "Editor/EditorCoreFixture.h"
#include "Editor/Panels/StatusBar.h"
#include "Editor/UIComponents/UIComponentsImGuiFixture.h"

#include <gtest/gtest.h>

#include <string>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class PanelStyleFixture : public EditorCoreFixture
{
protected:
    EditorTheme m_theme{};

    void SetUp() override
    {
        EditorCoreFixture::SetUp();
        EditorChrome_CreateTestImGuiContext(ImVec2(1280.f, 720.f));
        PopulateTestEditorThemeColors(m_theme.colors);
    }

    void TearDown() override
    {
        EditorChrome_DestroyTestImGuiContext();
        EditorCoreFixture::TearDown();
    }
};

TEST_F(PanelStyleFixture, AppHeader_Draw_WithValidContext_CompletesFrame)
{
    EditorChromeContext ctx{ &m_theme, m_core.get(), nullptr };
    EXPECT_TRUE(EditorChrome_Test_DrawAppHeader(ctx));
}

TEST_F(PanelStyleFixture, AppHeader_Draw_WithNullTheme_ReturnsEarly)
{
    EditorChromeContext ctx{ nullptr, m_core.get(), nullptr };
    EXPECT_TRUE(EditorChrome_Test_DrawAppHeader(ctx));
}

TEST_F(PanelStyleFixture, MainToolbar_Draw_WithValidContext_CompletesFrame)
{
    EditorChromeContext ctx{ &m_theme, m_core.get(), nullptr };
    EXPECT_TRUE(EditorChrome_Test_DrawMainToolbar(ctx));
}

TEST_F(PanelStyleFixture, StatusBar_Draw_WithLongStrings_TruncatesSummaryWithoutThrow)
{
    StatusBar bar;
    bar.rendererName  = std::string(5000, 'R');
    bar.buildConfig   = std::string(5000, 'B');
    bar.engineVersion = std::string(5000, 'E');

    EditorChromeContext ctx{ &m_theme, m_core.get(), nullptr };

    EXPECT_TRUE(EditorChrome_Test_DrawStatusBar(ctx, &bar));
}

TEST_F(PanelStyleFixture, AppHeader_Draw_NullCore_ValidTheme_NoCrashRegression_EditMenuGuard)
{
    EditorChromeContext ctx{ &m_theme, nullptr, nullptr };
    EXPECT_TRUE(EditorChrome_Test_DrawAppHeader(ctx));
}
