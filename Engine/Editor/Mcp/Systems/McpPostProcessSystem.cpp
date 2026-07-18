#include "Editor/Mcp/Systems/McpPostProcessSystem.h"

#include "Editor/EditorCore.h"
#include "Editor/Mcp/McpProtocol.h"
#include "Editor/Mcp/McpRegistry.h"

#include <nlohmann/json.hpp>

using namespace DeltaEngine;

void McpPostProcessSystem::RegisterTools(McpRegistry& registry)
{
    registry.RegisterCommand("post_process", "AddPass",
        [this](EditorCore& c, const nlohmann::json& p) { return CommandAddPass(c, p); });
    registry.RegisterCommand("post_process", "RemovePass",
        [this](EditorCore& c, const nlohmann::json& p) { return CommandRemovePass(c, p); });
}

nlohmann::json McpPostProcessSystem::CommandAddPass(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("assetId"))
        return MakeMcpError("missing required param: assetId");
    if (!params.contains("passClass"))
        return MakeMcpError("missing required param: passClass");

    nlohmann::json data;
    data["assetId"] = params["assetId"].get<std::string>();
    data["className"] = params["passClass"].get<std::string>();
    return EnqueueMcpCommand(core, "post_process", "EditorCommand_AddPostProcessPass", std::move(data), true);
}

nlohmann::json McpPostProcessSystem::CommandRemovePass(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("assetId"))
        return MakeMcpError("missing required param: assetId");
    if (!params.contains("passClass"))
        return MakeMcpError("missing required param: passClass");

    nlohmann::json data;
    data["assetId"] = params["assetId"].get<std::string>();
    data["className"] = params["passClass"].get<std::string>();
    return EnqueueMcpCommand(core, "post_process", "EditorCommand_RemovePostProcessPass", std::move(data), true);
}
