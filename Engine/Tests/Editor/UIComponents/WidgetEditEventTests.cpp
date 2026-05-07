#include "Editor/UIComponents/WidgetEditEvent.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

TEST(UIComponents_WidgetEditEvent, Merge_CombinesAllFlags)
{
    WidgetEditEvent a;
    WidgetEditEvent b;
    b.valueChanged = true;
    b.editBegan    = true;
    b.editEnded    = true;

    a.Merge(b);

    EXPECT_TRUE(a.valueChanged);
    EXPECT_TRUE(a.editBegan);
    EXPECT_TRUE(a.editEnded);
}

TEST(UIComponents_WidgetEditEvent, OperatorBool_IsTrueWhenValueChanged)
{
    WidgetEditEvent e;

    EXPECT_FALSE(static_cast<bool>(e));

    e.valueChanged = true;

    EXPECT_TRUE(static_cast<bool>(e));
}
