#include "Editor/UIComponents/UIComponentsImGuiFixture.h"

#include "Editor/UIComponents/ContextMenuPopup.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

TEST_F(UIComponentsImGuiFixture, ContextMenuPopup_Draw_EmptyOpen_NoCrash)
{
    ContextMenuPopup m;
    m.Open({});

    m.Draw(m_theme.colors);
}

TEST_F(UIComponentsImGuiFixture, ContextMenuPopup_Draw_NullLabel_SkipsItem)
{
    ContextMenuPopup m;
    m.Open({{nullptr, nullptr}});

    m.Draw(m_theme.colors);
}
