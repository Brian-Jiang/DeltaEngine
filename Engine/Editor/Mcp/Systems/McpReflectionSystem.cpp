#include "McpReflectionSystem.h"

#include "Mcp/McpRegistry.h"

using namespace DeltaEngine;

void McpReflectionSystem::RegisterTools(McpRegistry& registry)
{
    registry.RegisterOperation("reflection", "classes",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryClasses(c, p); });
    registry.RegisterOperation("reflection", "class_schema",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryClassSchema(c, p); });
    registry.RegisterOperation("reflection", "inheritance_chain",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryInheritanceChain(c, p); });
    registry.RegisterOperation("reflection", "find_classes_with_property",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryFindClassesWithProperty(c, p); });
}

nlohmann::json McpReflectionSystem::QueryClasses(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}

nlohmann::json McpReflectionSystem::QueryClassSchema(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}

nlohmann::json McpReflectionSystem::QueryInheritanceChain(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}

nlohmann::json McpReflectionSystem::QueryFindClassesWithProperty(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}
