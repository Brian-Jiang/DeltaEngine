#pragma once

#include "EditorIncludes.h"

DELTA_ENGINE_NS_BEGIN

class DProperty;

DELTAEDITOR_API int64_t ReadEnumUnderlyingAsInt64(const DProperty* prop, const void* addr);
DELTAEDITOR_API void WriteEnumUnderlyingFromInt64(const DProperty* prop, void* addr, int64_t value);

DELTA_ENGINE_NS_END
