#include "McpLightsSystem.h"

#include "EditorCore.h"
#include "Animation/EditorAnimationManager.h"
#include "Commands/EditorCommand_SetProperty.h"
#include "Commands/EditorCommandContext.h"
#include "Commands/EditorCommandManager.h"
#include "Mcp/McpRegistry.h"

#include "Runtime/Core/UUID.h"
#include "Runtime/Graphics/Light/LightComponent.h"

using namespace DeltaEngine;

void McpLightsSystem::RegisterTools(McpRegistry& registry)
{
    registry.RegisterOperation("lights", "SetIntensity",
        [this](EditorCore& c, const nlohmann::json& p) { return CommandSetIntensity(c, p); });
}

nlohmann::json McpLightsSystem::CommandSetIntensity(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("assetId") || !params.contains("objectId") || !params.contains("value"))
        return {{"ok", false}, {"error", "required params: assetId, objectId, value"}};

    const AssetId  assetId  = DeltaEngine::UUID::FromString(params["assetId"].get<std::string>());
    if (assetId.IsNull())
        return {{"ok", false}, {"error", "invalid assetId"}};

    const ObjectId objectId = DeltaEngine::UUID::FromString(params["objectId"].get<std::string>());
    if (objectId.IsNull())
        return {{"ok", false}, {"error", "invalid objectId"}};

    const float    target   = params["value"].get<float>();
    const float    duration = params.value("duration_seconds", 0.0f);

    LightComponent* light = core.ResolveObject<LightComponent>(assetId, objectId);
    if (!light)
        return {{"ok", false}, {"error", "object not found or not a LightComponent"}};

    EditorAnimationManager* animMgr = core.GetAnimationManager();

    if (animMgr == nullptr || duration <= 0.0f)
    {
        const float current = light->GetIntensity();
        EditorCommandContext ctx{core};
        auto cmd = std::make_unique<EditorCommand_SetProperty>(
            assetId, objectId, "m_intensity",
            nlohmann::json(current),
            nlohmann::json(target));
        core.GetCommandManager().Execute(std::move(cmd), ctx);
        return {{"ok", true}};
    }

    const float current = light->GetIntensity();
    animMgr->StartAnimation(
        assetId, objectId, "m_intensity",
        current, target, duration,
        [light](float v) { light->SetIntensity(v); });

    return {{"ok", true}};
}
