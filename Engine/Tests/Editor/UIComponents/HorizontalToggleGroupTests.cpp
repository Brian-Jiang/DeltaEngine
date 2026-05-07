#include "Editor/UIComponents/UIComponentsImGuiFixture.h"

#include "Editor/UIComponents/HorizontalToggleGroup.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

TEST_F(UIComponentsImGuiFixture, HorizontalToggleGroup_Draw_NullId_ReturnsEmptyEvent)
{
    HorizontalToggleGroup     g;
    HorizontalToggleGroup::Item items[] = {{"A", nullptr}};
    int                       sel = 0;
    WidgetEditEvent           evt = g.Draw(nullptr, m_theme.colors, items, 1, sel);
    EXPECT_FALSE(evt.valueChanged);
}

TEST_F(UIComponentsImGuiFixture, HorizontalToggleGroup_Draw_NonPositiveCount_ReturnsEmptyEvent)
{
    HorizontalToggleGroup     g;
    HorizontalToggleGroup::Item items[] = {{"A", nullptr}};
    int                       sel = 0;
    WidgetEditEvent           e1 = g.Draw("id", m_theme.colors, nullptr, 0, sel);
    WidgetEditEvent           e2 = g.Draw("id", m_theme.colors, items, 0, sel);
    EXPECT_FALSE(e1.valueChanged);
    EXPECT_FALSE(e2.valueChanged);
}
