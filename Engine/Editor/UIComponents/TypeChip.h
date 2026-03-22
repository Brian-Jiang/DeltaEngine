#pragma once

#include "EngineIncludes.h"

#include "imgui.h"
#include "Style/EditorTheme.h"

DELTA_ENGINE_NS_BEGIN

class TypeChip
{
public:
    // Small type icon chip; advances layout — use SameLine after if needed.
    void Draw(const EditorTheme::ThemeColors& c);
};

DELTA_ENGINE_NS_END
