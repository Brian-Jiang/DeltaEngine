#include "Editor/Commands/EditorCommandContext.h"

#include "Runtime/Core/DObject.h"
#include "Runtime/Reflection/DProperty.h"

using namespace DeltaEngine;

void EditorCommandContext::ApplyReflectedWrite(DObject* obj, DProperty* prop, const void* value)
{
    DELTA_ASSERT_MSG(obj != nullptr, "ApplyReflectedWrite requires non-null object");
    DELTA_ASSERT_MSG(prop != nullptr, "ApplyReflectedWrite requires non-null property");
    DELTA_ASSERT_MSG(value != nullptr, "ApplyReflectedWrite requires non-null value pointer");
    prop->SetValue(obj, value);
    obj->PostEditChangeProperty(prop);
}
