#pragma once

#include "EditorIncludes.h"

#include "Runtime/Reflection/DProperty.h"

#include <string>

DELTA_ENGINE_NS_BEGIN

DELTAEDITOR_API std::string FormatPropertyInspectorLabel(const std::string& propName);

DELTAEDITOR_API bool IsUndoableInspectorPropertyType(EPropertyType type);

DELTA_ENGINE_NS_END
