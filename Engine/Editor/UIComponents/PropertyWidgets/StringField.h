#pragma once

#include "EngineIncludes.h"
#include "imgui.h"

#include <cstddef>

DELTA_ENGINE_NS_BEGIN

class StringField
{
public:
    // Labeled text field; returns true when the buffer changed this frame.
    bool Draw(const char* label, char* buf, size_t bufSize, bool readOnly = false);
};

DELTA_ENGINE_NS_END
