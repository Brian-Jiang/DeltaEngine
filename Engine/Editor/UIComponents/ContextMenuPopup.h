#pragma once

#include "EngineIncludes.h"

#include <functional>
#include <vector>

#include "Style/EditorTheme.h"

DELTA_ENGINE_NS_BEGIN

class ContextMenuPopup
{
public:
    struct Item
    {
        const char* label;
        std::function<void()> action;
    };

    // Call when the context popup is open (e.g. inside BeginPopupContextItem).
    // Stores items for this popup instance.
    void Open(std::vector<Item> items);

    // Renders menu items. Call from within BeginPopupContextItem.
    // Invokes the action when a menu item is clicked.
    void Draw(const EditorTheme::ThemeColors& c);

private:
    std::vector<Item> m_items;
};

DELTA_ENGINE_NS_END
