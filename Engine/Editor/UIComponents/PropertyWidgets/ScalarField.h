#pragma once

#include "EditorIncludes.h"
#include "UIComponents/WidgetEditEvent.h"
#include "imgui.h"

DELTA_ENGINE_NS_BEGIN

class DELTAEDITOR_API ScalarField
{
public:
    WidgetEditEvent Draw(const char* label, float* value, float speed = 0.1f, const char* fmt = "%.3f");
};

DELTA_ENGINE_NS_END
