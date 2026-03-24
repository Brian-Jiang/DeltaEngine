#pragma once

#include "EditorIncludes.h"

DELTA_ENGINE_NS_BEGIN

struct WidgetEditEvent
{
    bool valueChanged = false;
    bool editBegan    = false;
    bool editEnded    = false;

    explicit operator bool() const { return valueChanged; }

    WidgetEditEvent& Merge(const WidgetEditEvent& other)
    {
        valueChanged |= other.valueChanged;
        editBegan    |= other.editBegan;
        editEnded    |= other.editEnded;
        return *this;
    }
};

DELTA_ENGINE_NS_END
