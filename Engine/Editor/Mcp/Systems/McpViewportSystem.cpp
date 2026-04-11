#include "McpViewportSystem.h"

#include "Mcp/McpRegistry.h"

using namespace DeltaEngine;

void McpViewportSystem::RegisterTools(McpRegistry& registry)
{
    registry.RegisterOperation("viewport", "camera",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryCamera(c, p); });
    registry.RegisterOperation("viewport", "render_settings",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryRenderSettings(c, p); });
    registry.RegisterOperation("viewport", "visible_objects",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryVisibleObjects(c, p); });
    registry.RegisterOperation("viewport", "raycast",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryRaycast(c, p); });
}

nlohmann::json McpViewportSystem::QueryCamera(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}

nlohmann::json McpViewportSystem::QueryRenderSettings(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}

nlohmann::json McpViewportSystem::QueryVisibleObjects(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}

nlohmann::json McpViewportSystem::QueryRaycast(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}
