#pragma once

#include "EngineIncludes.h"
#include "UIComponents/WidgetEditEvent.h"
#include "imgui.h"

#include <cstddef>

DELTA_ENGINE_NS_BEGIN

class StringField
{
public:
    WidgetEditEvent Draw(const char* label, char* buf, size_t bufSize, bool readOnly = false);
};

DELTA_ENGINE_NS_END
