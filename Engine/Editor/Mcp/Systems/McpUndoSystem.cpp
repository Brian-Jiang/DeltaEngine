#include "McpUndoSystem.h"

#include "Mcp/McpRegistry.h"

using namespace DeltaEngine;

void McpUndoSystem::RegisterTools(McpRegistry& registry)
{
    registry.RegisterOperation("undo_history", "stack",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryStack(c, p); });
}

nlohmann::json McpUndoSystem::QueryStack(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}
