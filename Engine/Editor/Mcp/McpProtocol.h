#pragma once

#include "EditorIncludes.h"

#include <nlohmann/json.hpp>

#include <string>
#include <string_view>

DELTA_ENGINE_NS_BEGIN

class EditorCore;

DELTAEDITOR_API nlohmann::json MakeMcpError(const std::string& message);

// Builds a command envelope and executes it synchronously on the calling (main)
// thread, returning the command's real result payload.
DELTAEDITOR_API nlohmann::json ExecuteMcpCommand(
    EditorCore& core,
    std::string_view system,
    const std::string& commandName,
    nlohmann::json params);

DELTA_ENGINE_NS_END
