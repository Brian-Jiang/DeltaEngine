#pragma once

#include "EditorIncludes.h"
#include "UIComponents/WidgetEditEvent.h"

DELTA_ENGINE_NS_BEGIN

class DEnum;

class DELTAEDITOR_API EnumField
{
public:
    WidgetEditEvent Draw(const char* label, int64_t* underlyingValue, DEnum* schema);
};

DELTA_ENGINE_NS_END
