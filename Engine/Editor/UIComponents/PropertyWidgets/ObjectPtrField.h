#pragma once

#include "EditorIncludes.h"

#include <optional>

DELTA_ENGINE_NS_BEGIN

class DObject;
class DClass;

class DELTAEDITOR_API ObjectPtrField
{
public:
    /// Returns an engaged optional when the user commits a selection (nullptr value = clear).
    std::optional<DObject*> Draw(const char* label, DObject* current, const DClass* targetClass, const char* popupId);

private:
    char m_pickerFilter[128] = {};
};

DELTA_ENGINE_NS_END
