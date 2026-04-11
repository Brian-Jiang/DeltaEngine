#include "McpSelectionSystem.h"

#include "Mcp/McpRegistry.h"

using namespace DeltaEngine;

void McpSelectionSystem::RegisterTools(McpRegistry& registry)
{
    registry.RegisterOperation("selection", "current",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryCurrent(c, p); });
}

nlohmann::json McpSelectionSystem::QueryCurrent(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}
