#include "McpProtocol.h"

#include "Editor/EditorCore.h"

#include <nlohmann/json.hpp>

DELTA_ENGINE_NS_BEGIN

nlohmann::json MakeMcpError(const std::string& message)
{
    return {{"ok", false}, {"error", message}};
}

nlohmann::json ExecuteMcpCommand(
    EditorCore& core,
    std::string_view system,
    const std::string& commandName,
    nlohmann::json params)
{
    nlohmann::json envelope;
    envelope["type"] = "command";
    envelope["system"] = system;
    envelope["command"] = commandName;
    envelope["params"] = std::move(params);

    return core.ExecuteSerializedCommand(envelope);
}

DELTA_ENGINE_NS_END
