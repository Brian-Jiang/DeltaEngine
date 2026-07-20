#include "McpCommonSystem.h"

#include "Editor/Commands/EditorAuxiliarySceneCommands.h"
#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/Commands/EditorCommand_RenameObject.h"
#include "Editor/Commands/EditorCommand_SetProperty.h"
#include "Editor/EditorCore.h"
#include "Mcp/McpProtocol.h"
#include "Mcp/McpRegistry.h"

#include "Runtime/Core/UUID.h"

#include <memory>
#include <nlohmann/json.hpp>

using namespace DeltaEngine;

namespace
{
// Resolves an explicit assetId param, falling back to the active scene asset.
AssetId ResolveAssetId(EditorCore& core, const nlohmann::json& params)
{
    if (params.contains("assetId") && params["assetId"].is_string())
    {
        const AssetId id = UUID::FromString(params["assetId"].get<std::string>());
        if (!id.IsNull())
            return id;
    }
    return ActiveSceneAssetId(core);
}
} // namespace

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

    const ObjectId targetObjectId = UUID::FromString(params["objectId"].get<std::string>());
    if (targetObjectId.IsNull())
        return MakeMcpError("invalid objectId");

    nlohmann::json err;
    if (!ExecuteMcpCommand(core, std::make_unique<EditorCommand_RenameObject>(
            ResolveAssetId(core, params), targetObjectId, params["newName"].get<std::string>()), err))
        return err;

    return MakeMcpOk();
}

nlohmann::json McpCommonSystem::CommandSetProperty(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("objectId"))
        return MakeMcpError("missing required param: objectId");
    if (!params.contains("propertyName"))
        return MakeMcpError("missing required param: propertyName");
    if (!params.contains("valueAfter"))
        return MakeMcpError("missing required param: valueAfter");

    const ObjectId objectId = UUID::FromString(params["objectId"].get<std::string>());
    if (objectId.IsNull())
        return MakeMcpError("invalid objectId");

    nlohmann::json err;
    if (!ExecuteMcpCommand(core, std::make_unique<EditorCommand_SetProperty>(
            ResolveAssetId(core, params), objectId,
            params["propertyName"].get<std::string>(),
            nlohmann::json{}, params["valueAfter"]), err))
        return err;

    return MakeMcpOk();
}

nlohmann::json McpCommonSystem::CommandSaveProject(EditorCore& core, const nlohmann::json&)
{
    EditorCommandContext ctx{core};
    core.GetCommandManager().ExecuteAuxiliary(
        std::make_unique<EditorAuxiliaryCommand_SaveScene>(), ctx);
    return {{"ok", true}, {"commandType", "SaveDirtyAssets"}};
}
