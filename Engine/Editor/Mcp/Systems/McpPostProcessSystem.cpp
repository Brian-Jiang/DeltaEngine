#include "Editor/Mcp/Systems/McpPostProcessSystem.h"

#include "Editor/Commands/EditorCommand_AddPostProcessPass.h"
#include "Editor/Commands/EditorCommand_RemovePostProcessPass.h"
#include "Editor/EditorCore.h"
#include "Editor/Mcp/McpProtocol.h"
#include "Editor/Mcp/McpRegistry.h"

#include "Runtime/Core/UUID.h"

#include <memory>
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

    const AssetId assetId = UUID::FromString(params["assetId"].get<std::string>());
    if (assetId.IsNull())
        return MakeMcpError("invalid assetId");

    return RunEditorCommand(core, std::make_unique<EditorCommand_AddPostProcessPass>(
        assetId, params["passClass"].get<std::string>()));
}

nlohmann::json McpPostProcessSystem::CommandRemovePass(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("assetId"))
        return MakeMcpError("missing required param: assetId");
    if (!params.contains("passClass"))
        return MakeMcpError("missing required param: passClass");

    const AssetId assetId = UUID::FromString(params["assetId"].get<std::string>());
    if (assetId.IsNull())
        return MakeMcpError("invalid assetId");

    return RunEditorCommand(core, std::make_unique<EditorCommand_RemovePostProcessPass>(
        assetId, params["passClass"].get<std::string>()));
}
