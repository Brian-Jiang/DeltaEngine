#pragma once

#include "EngineIncludes.h"
#include "imgui.h"

DELTA_ENGINE_NS_BEGIN

class ReferenceField
{
public:
    // Read-only object reference row; shows displayName or "(null)" when isNull.
    void Draw(const char* label, const char* displayName, bool isNull = false);
};

DELTA_ENGINE_NS_END
