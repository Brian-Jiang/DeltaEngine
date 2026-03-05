#pragma once

#include "EngineIncludes.h"
#include "imgui.h"

#include <cstddef>

DELTA_ENGINE_NS_BEGIN

class StringField
{
public:
    // Draws a labeled text input field.
    // buf / bufSize — character buffer; readOnly — disallow editing.
    // Returns true if the buffer was modified this frame.
    bool Draw(const char* label, char* buf, size_t bufSize, bool readOnly = false);
};

DELTA_ENGINE_NS_END
