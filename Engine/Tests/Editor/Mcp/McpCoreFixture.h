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
        auto* reg = m_core->GetMcpRegistry();
        if (reg->HasQuery(system, op))
            return reg->DispatchQuery(system, op, *m_core, params);
        if (reg->HasCommand(system, op))
            return reg->DispatchCommand(system, op, *m_core, params);
        // Match prior behaviour: dispatch as a query so callers get the
        // unknown-system / unknown-name error envelope.
        return reg->DispatchQuery(system, op, *m_core, params);
    }

    nlohmann::json DispatchQuery(const std::string& system,
                                  const std::string& query,
                                  const nlohmann::json& params = nlohmann::json::object()) const
    {
        return m_core->GetMcpRegistry()->DispatchQuery(system, query, *m_core, params);
    }

    nlohmann::json DispatchCommand(const std::string& system,
                                    const std::string& command,
                                    const nlohmann::json& params = nlohmann::json::object()) const
    {
        return m_core->GetMcpRegistry()->DispatchCommand(system, command, *m_core, params);
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
        return m_core->ExecuteSerializedCommand(env).value("objectId", std::string{});
    }
};

} // namespace DeltaEngine::Tests
