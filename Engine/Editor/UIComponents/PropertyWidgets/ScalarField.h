#pragma once

#include "EngineIncludes.h"
#include "UIComponents/WidgetEditEvent.h"
#include "imgui.h"

DELTA_ENGINE_NS_BEGIN

class ScalarField
{
public:
    WidgetEditEvent Draw(const char* label, float* value, float speed = 0.1f, const char* fmt = "%.3f");
};

DELTA_ENGINE_NS_END
