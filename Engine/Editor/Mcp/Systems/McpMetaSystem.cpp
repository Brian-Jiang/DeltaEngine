#include "McpMetaSystem.h"

#include "Mcp/McpRegistry.h"

using namespace DeltaEngine;

void McpMetaSystem::RegisterTools(McpRegistry& registry)
{
    registry.RegisterOperation("meta", "list_operations",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryListOperations(c, p); });
    registry.RegisterOperation("meta", "describe_operations",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryDescribeOperations(c, p); });
    registry.RegisterOperation("meta", "capabilities",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryCapabilities(c, p); });
    registry.RegisterOperation("meta", "active_systems",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryActiveSystems(c, p); });
}

nlohmann::json McpMetaSystem::QueryListOperations(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}

nlohmann::json McpMetaSystem::QueryDescribeOperations(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}

nlohmann::json McpMetaSystem::QueryCapabilities(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}

nlohmann::json McpMetaSystem::QueryActiveSystems(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}
