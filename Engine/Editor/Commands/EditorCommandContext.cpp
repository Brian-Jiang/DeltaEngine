#include "Editor/Commands/EditorCommandContext.h"
#include "Runtime/Core/DObject.h"
#include "Runtime/Reflection/DProperty.h"

using namespace DeltaEngine;

void EditorCommandContext::ApplyReflectedWrite(DObject* obj, DProperty* prop, const void* value)
{
    prop->SetValue(obj, value);
    obj->PostEditChangeProperty(prop);
}
