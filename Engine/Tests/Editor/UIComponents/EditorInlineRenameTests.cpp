#include "Editor/UIComponents/UIComponentsImGuiFixture.h"

#include "Editor/UIComponents/EditorInlineRename.h"

#include <gtest/gtest.h>

#include <cstring>
#include <string>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

TEST_F(UIComponentsImGuiFixture, EditorInlineRename_IsInactive_ReturnsNoneOnDraw)
{
    EditorInlineRename ren;

    EXPECT_EQ(ren.Draw(), EditorInlineRename::Result::None);
}

TEST_F(UIComponentsImGuiFixture, EditorInlineRename_Begin_TruncatesLongNameToBuffer)
{
    EditorInlineRename ren;
    const std::string   longName(700, 'a');

    ren.Begin(longName);

    EXPECT_TRUE(ren.IsActive());
    EXPECT_EQ(std::strlen(ren.GetBuffer()), 511u);
}

TEST_F(UIComponentsImGuiFixture, EditorInlineRename_Clear_ResetsState)
{
    EditorInlineRename ren;

    ren.Begin("x");
    ren.Clear();

    EXPECT_FALSE(ren.IsActive());
    EXPECT_STREQ(ren.GetBuffer(), "");
}
