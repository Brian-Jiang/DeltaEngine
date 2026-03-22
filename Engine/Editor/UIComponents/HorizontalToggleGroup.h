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

    // Returns true when `selected` changes; optional override colors the selected slot when indices match.
    bool Draw(const char* id, const Item* items, int itemCount,
              int& selected, float itemW = 0.f, float itemH = 0.f,
              const int* overrideSelectedIndex = nullptr,
              const ImVec4* overrideSelectedColor = nullptr);
};

DELTA_ENGINE_NS_END
