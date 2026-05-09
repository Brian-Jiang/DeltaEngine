#include "Editor/EditorWindows/EditorDetailsPropertyFormatting.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

TEST(EditorDetailsPropertyFormattingTests, FormatPropertyInspectorLabel_MPrefix_SplitsCamelCaseWords)
{
    EXPECT_EQ(FormatPropertyInspectorLabel("m_fooBar"), "Foo Bar");
}

TEST(EditorDetailsPropertyFormattingTests, FormatPropertyInspectorLabel_Empty_ReturnsEmpty)
{
    EXPECT_TRUE(FormatPropertyInspectorLabel({}).empty());
}

TEST(EditorDetailsPropertyFormattingTests, IsUndoableInspectorPropertyType_Float_ReturnsTrue)
{
    EXPECT_TRUE(IsUndoableInspectorPropertyType(EPropertyType::Float));
}

TEST(EditorDetailsPropertyFormattingTests, IsUndoableInspectorPropertyType_ObjectPtr_ReturnsFalse)
{
    EXPECT_FALSE(IsUndoableInspectorPropertyType(EPropertyType::ObjectPtr));
}
