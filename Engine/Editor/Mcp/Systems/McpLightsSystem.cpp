#include "Editor/Mcp/Systems/McpLightsSystem.h"

#include "Editor/EditorCore.h"
#include "Editor/Mcp/McpAnimationDefaults.h"
#include "Editor/Mcp/McpProtocol.h"
#include "Editor/Mcp/McpRegistry.h"

#include "Runtime/Core/UUID.h"

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

    const AssetId  assetId  = DeltaEngine::UUID::FromString(params["assetId"].get<std::string>());
    if (assetId.IsNull())
        return MakeMcpError("invalid assetId");

    const ObjectId objectId = DeltaEngine::UUID::FromString(params["objectId"].get<std::string>());
    if (objectId.IsNull())
        return MakeMcpError("invalid objectId");

    const float target   = params["value"].get<float>();
    //const float duration = params.value("duration_seconds", kDefaultAnimationDurationSeconds);
    const float duration = kDefaultAnimationDurationSeconds;

    if (duration <= 0.0f)
    {
        // Immediate path: enqueue EditorCommand_SetProperty for main-thread execution via DrainCommandQueue.
        nlohmann::json envelope;
        envelope["type"]    = "command";
        envelope["command"] = "EditorCommand_SetProperty";
        envelope["params"]  = {
            {"assetId",      assetId.ToString()},
            {"objectId",     objectId.ToString()},
            {"propertyName", "m_intensity"},
            {"valueAfter",   target}
        };
        core.EnqueueSerializedCommand(envelope.dump());
        return {{"ok", true}, {"queued", true}, {"expects_result", true}};
    }
    else
    {
        // Animation path: enqueue auxiliary for main-thread execution via DrainCommandQueue.
        nlohmann::json envelope;
        envelope["type"]         = "auxiliary";
        envelope["name"]         = "StartLightAnimation";
        envelope["assetId"]      = assetId.ToString();
        envelope["objectId"]     = objectId.ToString();
        envelope["propertyName"] = "m_intensity";
        envelope["targetValue"]  = target;
        envelope["duration"]     = duration;
        core.EnqueueSerializedCommand(envelope.dump());
        return {{"ok", true}, {"queued", true}, {"expects_result", false}};
    }
}
