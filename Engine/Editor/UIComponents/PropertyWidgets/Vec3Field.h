#pragma once

#include "EngineIncludes.h"
#include "imgui.h"

DELTA_ENGINE_NS_BEGIN

class Vec3Field
{
public:
    // Draws a labeled three-component drag field with colored X/Y/Z axis chips.
    // values — pointer to float[3]; speed — drag sensitivity; fmt — format string.
    // Returns true if any component was modified this frame.
    bool Draw(const char* label, float* values, float speed = 0.1f, const char* fmt = "%.3f");
};

DELTA_ENGINE_NS_END
