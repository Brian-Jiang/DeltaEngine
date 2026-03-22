#pragma once

#include "EngineIncludes.h"
#include "imgui.h"

DELTA_ENGINE_NS_BEGIN

class ColorField
{
public:
    // Labeled RGBA editor; `values` is float[4] in RGBA order.
    bool Draw(const char* label, float* values, bool hasAlpha = false);
};

DELTA_ENGINE_NS_END
