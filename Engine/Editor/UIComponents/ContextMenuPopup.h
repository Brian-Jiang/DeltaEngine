#pragma once

#include "EditorIncludes.h"

#include <functional>
#include <vector>

#include "Style/EditorTheme.h"

DELTA_ENGINE_NS_BEGIN

class DELTAEDITOR_API ContextMenuPopup
{
public:
    struct Item
    {
        const char* label;              // MenuItem text
        std::function<void()> action;   // Invoked on click
    };

    // Replaces the menu entries for the next Draw().
    void Open(std::vector<Item> items);

    // Renders queued items; call inside the active popup scope.
    void Draw(const EditorTheme::ThemeColors& c);

private:
    std::vector<Item> m_items;
};

DELTA_ENGINE_NS_END
