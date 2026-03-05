#pragma once

#include "EngineIncludes.h"
#include "imgui.h"

DELTA_ENGINE_NS_BEGIN

class ScalarField
{
public:
    // Draws a labeled single-float drag field with themed input background.
    // Returns true if the value was modified this frame.
    bool Draw(const char* label, float* value, float speed = 0.1f, const char* fmt = "%.3f");
};

DELTA_ENGINE_NS_END
