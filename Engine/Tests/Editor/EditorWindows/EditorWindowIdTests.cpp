#include "Editor/EditorWindows/EditorWindow.h"

#include <gtest/gtest.h>

#include <string>

using namespace DeltaEngine;

namespace
{
struct IdTestEditorWindow final : EditorWindow
{
    void Render(bool& open) override { (void)open; }
};
}

TEST(EditorWindows, EditorWindow_SetWindowId_Positive_AssignsImGuiTitleSuffix)
{
    IdTestEditorWindow w;
    w.m_title = "TestTitle";
    w.SetWindowId(3);

    EXPECT_EQ(w.m_id, 3);
    EXPECT_EQ(std::string(w.GetImGuiTitle()), "TestTitle##3");
}
