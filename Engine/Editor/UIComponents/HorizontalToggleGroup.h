#pragma once

#include "EngineIncludes.h"

#include "imgui.h"

DELTA_ENGINE_NS_BEGIN

class HorizontalToggleGroup
{
public:
    struct Item
    {
        const char* label;
        const char* tooltip;
    };

    // items     — array of Item descriptors
    // itemCount — length of items array
    // selected  — index of the currently active item (in/out)
    // itemW     — width of each button (0 = auto/equal)
    // itemH     — height of each button
    // overrideSelectedIndex — when set and selected == *overrideSelectedIndex, use overrideSelectedColor
    // overrideSelectedColor — color for text/border when override applies (e.g. Ok green for Play)
    // Returns true if selection changed this frame.
    bool Draw(const char* id, const Item* items, int itemCount,
              int& selected, float itemW = 28.f, float itemH = 24.f,
              const int* overrideSelectedIndex = nullptr,
              const ImVec4* overrideSelectedColor = nullptr);
};

DELTA_ENGINE_NS_END
