#include "McpProjectSystem.h"

#include "Mcp/McpRegistry.h"

using namespace DeltaEngine;

void McpProjectSystem::RegisterTools(McpRegistry& registry)
{
    registry.RegisterOperation("project", "info",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryInfo(c, p); });
    registry.RegisterOperation("project", "settings",
        [this](EditorCore& c, const nlohmann::json& p) { return QuerySettings(c, p); });
    registry.RegisterOperation("project", "open_scenes",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryOpenScenes(c, p); });
    registry.RegisterOperation("project", "build_state",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryBuildState(c, p); });
}

nlohmann::json McpProjectSystem::QueryInfo(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}

nlohmann::json McpProjectSystem::QuerySettings(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}

nlohmann::json McpProjectSystem::QueryOpenScenes(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}

nlohmann::json McpProjectSystem::QueryBuildState(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}
