#include "McpProtocol.h"

#include "Editor/Commands/EditorCommand.h"
#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/EditorCore.h"

#include "Runtime/Assets/DPrimaryAsset.h"

#include <nlohmann/json.hpp>

DELTA_ENGINE_NS_BEGIN

nlohmann::json MakeMcpError(const std::string& message)
{
    return {{"ok", false}, {"error", message}};
}

nlohmann::json MakeMcpOk()
{
    return {{"ok", true}};
}

bool ExecuteMcpCommand(EditorCore& core, std::unique_ptr<EditorCommand> cmd, nlohmann::json& outError)
{
    if (!cmd)
    {
        outError = MakeMcpError("null command");
        return false;
    }

    EditorCommandContext ctx{core};
    std::string typeName{cmd->GetTypeName()};

    if (!core.GetCommandManager().Execute(std::move(cmd), ctx))
    {
        outError = {{"ok", false}, {"commandType", typeName}, {"error", "Execute() returned false"}};
        return false;
    }

    return true;
}

AssetId ActiveSceneAssetId(EditorCore& core)
{
    DPrimaryAsset* activeAsset = core.GetActiveSceneAsset();
    return activeAsset ? activeAsset->GetAssetId() : AssetId{};
}

DELTA_ENGINE_NS_END
