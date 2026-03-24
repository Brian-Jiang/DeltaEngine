#pragma once

#include "EngineIncludes.h"
#include "UIComponents/WidgetEditEvent.h"
#include "imgui.h"

DELTA_ENGINE_NS_BEGIN

class ColorField
{
public:
    WidgetEditEvent Draw(const char* label, float* values, bool hasAlpha = false);
};

DELTA_ENGINE_NS_END
