#pragma once

#include "EngineIncludes.h"

#include "Core/UUID.h"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

DELTA_ENGINE_NS_BEGIN

struct ObjectSnapshot
{
    nlohmann::json rootJson;
    std::vector<ObjectId> capturedIds;
    std::string rootClassName;
};

DELTA_ENGINE_NS_END
