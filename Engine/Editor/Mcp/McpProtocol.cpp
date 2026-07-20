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

nlohmann::json RunEditorCommand(EditorCore& core, std::unique_ptr<EditorCommand> cmd)
{
    if (!cmd)
        return MakeMcpError("null command");

    EditorCommandContext ctx{core};
    std::string typeName{cmd->GetTypeName()};
    EditorCommand* cmdPtr = cmd.get();

    if (!core.GetCommandManager().Execute(std::move(cmd), ctx))
        return {{"ok", false}, {"commandType", typeName}, {"error", "Execute() returned false"}};

    nlohmann::json j;
    cmdPtr->Serialize(j);
    std::string objectId = j.value("createdId", "");
    if (objectId.empty())
        objectId = j.value("createdComponentId", "");
    return {{"ok", true}, {"commandType", typeName}, {"objectId", objectId}};
}

AssetId ActiveSceneAssetId(EditorCore& core)
{
    DPrimaryAsset* activeAsset = core.GetActiveSceneAsset();
    return activeAsset ? activeAsset->GetAssetId() : AssetId{};
}

DELTA_ENGINE_NS_END
