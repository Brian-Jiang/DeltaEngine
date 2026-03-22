#pragma once

#include "EngineIncludes.h"
#include "imgui.h"

DELTA_ENGINE_NS_BEGIN

class ScalarField
{
public:
    // Labeled float drag; returns true when the value changed this frame.
    bool Draw(const char* label, float* value, float speed = 0.1f, const char* fmt = "%.3f");
};

DELTA_ENGINE_NS_END
