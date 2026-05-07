#pragma once

#include "EditorIncludes.h"
#include "UIComponents/WidgetEditEvent.h"
#include "imgui.h"

DELTA_ENGINE_NS_BEGIN

class DELTAEDITOR_API ColorField
{
public:
    WidgetEditEvent Draw(const char* label, float* values, bool hasAlpha = false);
};

DELTA_ENGINE_NS_END
