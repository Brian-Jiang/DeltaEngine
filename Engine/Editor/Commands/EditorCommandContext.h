#pragma once

#include "EditorIncludes.h"

DELTA_ENGINE_NS_BEGIN

class DObject;
class DProperty;
class EditorCore;

struct EditorCommandContext
{
    EditorCore& core;

    static void ApplyReflectedWrite(DObject* obj, DProperty* prop, const void* value);
};

DELTA_ENGINE_NS_END
