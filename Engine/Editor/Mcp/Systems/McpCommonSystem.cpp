#include "McpCommonSystem.h"

#include "Editor/EditorCore.h"
#include "Mcp/McpProtocol.h"
#include "Mcp/McpRegistry.h"

#include <nlohmann/json.hpp>

using namespace DeltaEngine;

void McpCommonSystem::RegisterTools(McpRegistry& registry)
{
    registry.RegisterCommand("common", "RenameObject",
        [this](EditorCore& c, const nlohmann::json& p) { return CommandRenameObject(c, p); });
    registry.RegisterCommand("common", "SetProperty",
        [this](EditorCore& c, const nlohmann::json& p) { return CommandSetProperty(c, p); });
    registry.RegisterCommand("common", "SaveProject",
        [this](EditorCore& c, const nlohmann::json& p) { return CommandSaveProject(c, p); });
}

nlohmann::json McpCommonSystem::CommandRenameObject(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("objectId"))
        return MakeMcpError("missing required param: objectId");
    if (!params.contains("newName"))
        return MakeMcpError("missing required param: newName");

    nlohmann::json data;
    data["targetObjectId"] = params["objectId"].get<std::string>();
    data["newName"] = params["newName"].get<std::string>();
    if (params.contains("assetId"))
        data["assetId"] = params["assetId"].get<std::string>();
    return ExecuteMcpCommand(core, "common", "EditorCommand_RenameObject", std::move(data));
}

nlohmann::json McpCommonSystem::CommandSetProperty(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("objectId"))
        return MakeMcpError("missing required param: objectId");
    if (!params.contains("propertyName"))
        return MakeMcpError("missing required param: propertyName");
    if (!params.contains("valueAfter"))
        return MakeMcpError("missing required param: valueAfter");

    nlohmann::json data;
    data["objectId"] = params["objectId"].get<std::string>();
    data["propertyName"] = params["propertyName"].get<std::string>();
    data["valueAfter"] = params["valueAfter"];
    if (params.contains("assetId"))
        data["assetId"] = params["assetId"].get<std::string>();
    return ExecuteMcpCommand(core, "common", "EditorCommand_SetProperty", std::move(data));
}

nlohmann::json McpCommonSystem::CommandSaveProject(EditorCore& core, const nlohmann::json&)
{
    nlohmann::json envelope;
    envelope["type"] = "auxiliary";
    envelope["name"] = "SaveDirtyAssets";
    return core.ExecuteSerializedCommand(envelope);
}
