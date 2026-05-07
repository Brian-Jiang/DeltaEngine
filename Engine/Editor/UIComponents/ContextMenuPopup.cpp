#include "UIComponents/ContextMenuPopup.h"

#include "UIComponents/UIComponentsEditorTheme.h"

#include "Style/EditorTheme.h"

#include "imgui.h"

using namespace DeltaEngine;

void ContextMenuPopup::Open(std::vector<Item> items)
{
    m_items = std::move(items);
}

void ContextMenuPopup::Draw(const EditorTheme::ThemeColors& c)
{
    (void)c;
    for (Item& item : m_items)
    {
        if (!item.label)
        {
            DLOG(LogUIComponents, ELogLevel::Warning,
                "ContextMenuPopup::Draw: menu item has null label (skipping item, action={})",
                static_cast<const void*>(&item.action));
            continue;
        }
        if (ImGui::MenuItem(item.label))
        {
            if (item.action)
                item.action();
            else
            {
                DLOG(LogUIComponents, ELogLevel::Warning,
                    "ContextMenuPopup::Draw: menu item '{}' has null action (no-op)",
                    item.label);
            }
        }
    }
}
