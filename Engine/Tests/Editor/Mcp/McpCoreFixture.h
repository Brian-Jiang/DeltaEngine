#pragma once

#include "Editor/EditorCoreFixture.h"

#include "Editor/Mcp/McpRegistry.h"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace DeltaEngine::Tests
{

class McpCoreFixture : public EditorCoreFixture
{
protected:
    nlohmann::json Dispatch(const std::string& system,
                            const std::string& op,
                            const nlohmann::json& params = nlohmann::json::object()) const
    {
        return m_core->GetMcpRegistry()->Dispatch(system, op, *m_core, params);
    }

    // Creates a GO via the legacy command path and returns its objectId.
    std::string CreateLegacyGameObject() const
    {
        const AssetId sceneId = GetActiveSceneAssetId();
        nlohmann::json data;
        data["sceneAssetId"] = sceneId.ToString();
        data["className"]    = "GameObject";
        nlohmann::json env;
        env["type"] = "EditorCommand_CreateGameObject";
        env["data"] = data;
        m_core->EnqueueSerializedCommand(env.dump());
        std::vector<std::string> responses;
        m_core->DrainCommandQueue(responses);
        if (responses.empty())
            return {};
        return nlohmann::json::parse(responses[0]).value("objectId", std::string{});
    }
};

} // namespace DeltaEngine::Tests
