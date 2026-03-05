#pragma once

#include "EngineIncludes.h"

#include "imgui.h"
#include "Style/EditorTheme.h"

DELTA_ENGINE_NS_BEGIN

class TypeChip
{
public:
    // Draws a unified 15x15 chip for any DObject.
    // Uses CMesh color family; icon is always the diamond glyph.
    // Advances cursor — caller does SameLine(0, 6) after.
    void Draw(const EditorTheme::ThemeColors& c);
};

DELTA_ENGINE_NS_END
