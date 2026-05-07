#pragma once

#include "EditorIncludes.h"
#include "UIComponents/WidgetEditEvent.h"

#include "imgui.h"

DELTA_ENGINE_NS_BEGIN

class DELTAEDITOR_API HorizontalToggleGroup
{
public:
    struct Item
    {
        const char* label;
        const char* tooltip;
    };

    WidgetEditEvent Draw(const char* id, const Item* items, int itemCount,
                         int& selected, float itemW = 0.f, float itemH = 0.f,
                         const int* overrideSelectedIndex = nullptr,
                         const ImVec4* overrideSelectedColor = nullptr);
};

DELTA_ENGINE_NS_END
