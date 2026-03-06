#include "UIComponents/ContextMenuPopup.h"

#include "Style/EditorTheme.h"

#include "imgui.h"

using namespace DeltaEngine;

void ContextMenuPopup::Open(std::vector<Item> items)
{
    m_items = std::move(items);
}

void ContextMenuPopup::Draw(const EditorTheme::ThemeColors& c)
{
    for (Item& item : m_items)
    {
        if (ImGui::MenuItem(item.label))
        {
            if (item.action)
                item.action();
        }
    }
}
