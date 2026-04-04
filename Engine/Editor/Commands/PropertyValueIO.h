#pragma once

#include "EditorIncludes.h"

#include <nlohmann/json.hpp>

DELTA_ENGINE_NS_BEGIN

class DObject;
class DProperty;

DELTAEDITOR_API nlohmann::json PropertyToJson(const DObject* obj, const DProperty* prop);

DELTAEDITOR_API bool SetPropertyFromJson(DObject* obj, const DProperty* prop, const nlohmann::json& value);

DELTA_ENGINE_NS_END
