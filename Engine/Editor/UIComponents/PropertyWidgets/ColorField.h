#pragma once

#include "EngineIncludes.h"
#include "imgui.h"

DELTA_ENGINE_NS_BEGIN

class ColorField
{
public:
    // Draws a labeled color editor with a swatch button and inline hex/RGB edit.
    // values — float[4] in RGBA order; hasAlpha — show/edit alpha channel.
    // Returns true if the color was modified this frame.
    bool Draw(const char* label, float* values, bool hasAlpha = false);
};

DELTA_ENGINE_NS_END
