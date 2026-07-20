#include "Editor/Mcp/Systems/McpLightsSystem.h"

#include "Editor/Animation/EditorAnimationManager.h"
#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/Commands/EditorCommand_SetProperty.h"
#include "Editor/EditorCore.h"
#include "Editor/Mcp/McpAnimationDefaults.h"
#include "Editor/Mcp/McpProtocol.h"
#include "Editor/Mcp/McpRegistry.h"

#include "Runtime/Core/UUID.h"
#include "Runtime/Graphics/Light/LightComponent.h"

#include <memory>

using namespace DeltaEngine;

void McpLightsSystem::RegisterTools(McpRegistry& registry)
{
    registry.RegisterCommand("lights", "SetIntensity",
        [this](EditorCore& c, const nlohmann::json& p) { return CommandSetIntensity(c, p); });
}

nlohmann::json McpLightsSystem::CommandSetIntensity(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("assetId") || !params.contains("objectId") || !params.contains("value"))
        return MakeMcpError("required params: assetId, objectId, value");

    const AssetId assetId = UUID::FromString(params["assetId"].get<std::string>());
    if (assetId.IsNull())
        return MakeMcpError("invalid assetId");

    const ObjectId objectId = UUID::FromString(params["objectId"].get<std::string>());
    if (objectId.IsNull())
        return MakeMcpError("invalid objectId");

    const float target   = params["value"].get<float>();
    //const float duration = params.value("duration_seconds", kDefaultAnimationDurationSeconds);
    const float duration = kDefaultAnimationDurationSeconds;

    static constexpr const char* kIntensityProp = "m_intensity";

    if (duration <= 0.0f)
    {
        // Immediate path: apply via an undoable SetProperty.
        return RunEditorCommand(core, std::make_unique<EditorCommand_SetProperty>(
            assetId, objectId, kIntensityProp, nlohmann::json{}, nlohmann::json(target)));
    }

    // Animation path.
    LightComponent* light = core.ResolveObject<LightComponent>(assetId, objectId);
    if (!light)
        return MakeMcpError("object not found or not a LightComponent");

    const float current = light->GetIntensity();
    if (auto* animMgr = core.GetAnimationManager())
    {
        animMgr->StartAnimation(
            assetId, objectId, kIntensityProp,
            current, target, duration,
            [light](float v) { light->SetIntensity(v); });
    }
    else
    {
        // Headless fallback: apply immediately via SetProperty.
        EditorCommandContext ctx{core};
        core.GetCommandManager().Execute(
            std::make_unique<EditorCommand_SetProperty>(
                assetId, objectId, kIntensityProp,
                nlohmann::json(current), nlohmann::json(target)),
            ctx);
    }
    return {{"ok", true}, {"commandType", "SetIntensity"}};
}
