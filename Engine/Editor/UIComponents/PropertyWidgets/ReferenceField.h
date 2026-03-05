#pragma once

#include "EngineIncludes.h"
#include "imgui.h"

DELTA_ENGINE_NS_BEGIN

class ReferenceField
{
public:
    // Draws a read-only labeled reference slot showing the object display name.
    // isNull — when true, renders "(null)" in dim text.
    void Draw(const char* label, const char* displayName, bool isNull = false);
};

DELTA_ENGINE_NS_END
