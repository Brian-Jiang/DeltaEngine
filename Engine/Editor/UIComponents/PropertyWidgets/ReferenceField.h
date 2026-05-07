#pragma once

#include "EditorIncludes.h"
#include "imgui.h"

DELTA_ENGINE_NS_BEGIN

class DELTAEDITOR_API ReferenceField
{
public:
    // Read-only object reference row; shows displayName or "(null)" when isNull.
    void Draw(const char* label, const char* displayName, bool isNull = false);
};

DELTA_ENGINE_NS_END
