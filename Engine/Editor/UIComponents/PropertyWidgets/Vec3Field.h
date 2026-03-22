#pragma once

#include "EngineIncludes.h"
#include "imgui.h"

DELTA_ENGINE_NS_BEGIN

class Vec3Field
{
public:
    // Labeled XYZ drags with axis-colored chips; `values` is float[3]. Returns true if any axis changed.
    bool Draw(const char* label, float* values, float speed = 0.1f, const char* fmt = "%.3f");
};

DELTA_ENGINE_NS_END
