#include "McpSceneSystem.h"

#include "Mcp/McpRegistry.h"

using namespace DeltaEngine;

void McpSceneSystem::RegisterTools(McpRegistry& registry)
{
    registry.RegisterOperation("scene", "game_objects",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryGameObjects(c, p); });
    registry.RegisterOperation("scene", "game_object",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryGameObject(c, p); });
    registry.RegisterOperation("scene", "hierarchy",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryHierarchy(c, p); });
    registry.RegisterOperation("scene", "component",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryComponent(c, p); });
    registry.RegisterOperation("scene", "components_on_object",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryComponentsOnObject(c, p); });
    registry.RegisterOperation("scene", "find_by_property",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryFindByProperty(c, p); });
}

nlohmann::json McpSceneSystem::QueryGameObjects(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}

nlohmann::json McpSceneSystem::QueryGameObject(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}

nlohmann::json McpSceneSystem::QueryHierarchy(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}

nlohmann::json McpSceneSystem::QueryComponent(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}

nlohmann::json McpSceneSystem::QueryComponentsOnObject(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}

nlohmann::json McpSceneSystem::QueryFindByProperty(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}
