#include "McpCommonSystem.h"

#include "Editor/EditorCore.h"
#include "Mcp/McpRegistry.h"

#include <nlohmann/json.hpp>

using namespace DeltaEngine;

static nlohmann::json MakeError(const std::string& msg)
{
    return { {"ok", false}, {"error", msg} };
}

static nlohmann::json EnqueueCommand(EditorCore& core, std::string_view system, const std::string& commandName, nlohmann::json params)
{
    nlohmann::json envelope;
    envelope["type"] = "command";
    envelope["system"] = system;
    envelope["command"] = commandName;
    envelope["params"] = std::move(params);

    core.EnqueueSerializedCommand(envelope.dump());
    return { {"ok", true}, {"queued", true}, {"command", commandName} };
}

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
        return MakeError("missing required param: objectId");
    if (!params.contains("newName"))
        return MakeError("missing required param: newName");

    nlohmann::json data;
    data["targetObjectId"] = params["objectId"].get<std::string>();
    data["newName"] = params["newName"].get<std::string>();
    if (params.contains("assetId"))
        data["assetId"] = params["assetId"].get<std::string>();
    return EnqueueCommand(core, "common", "EditorCommand_RenameObject", std::move(data));
}

nlohmann::json McpCommonSystem::CommandSetProperty(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("objectId"))
        return MakeError("missing required param: objectId");
    if (!params.contains("propertyName"))
        return MakeError("missing required param: propertyName");
    if (!params.contains("valueAfter"))
        return MakeError("missing required param: valueAfter");

    nlohmann::json data;
    data["objectId"] = params["objectId"].get<std::string>();
    data["propertyName"] = params["propertyName"].get<std::string>();
    data["valueAfter"] = params["valueAfter"];
    if (params.contains("assetId"))
        data["assetId"] = params["assetId"].get<std::string>();
    return EnqueueCommand(core, "common", "EditorCommand_SetProperty", std::move(data));
}

nlohmann::json McpCommonSystem::CommandSaveProject(EditorCore& core, const nlohmann::json&)
{
    nlohmann::json envelope;
    envelope["type"] = "auxiliary";
    envelope["name"] = "SaveDirtyAssets";
    core.EnqueueSerializedCommand(envelope.dump());
    return { {"ok", true}, {"queued", true}, {"command", "SaveDirtyAssets"} };
}
