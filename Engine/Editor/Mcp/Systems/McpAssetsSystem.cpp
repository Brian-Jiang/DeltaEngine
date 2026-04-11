#include "McpAssetsSystem.h"

#include "Mcp/McpRegistry.h"

using namespace DeltaEngine;

void McpAssetsSystem::RegisterTools(McpRegistry& registry)
{
    registry.RegisterOperation("assets", "list",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryList(c, p); });
    registry.RegisterOperation("assets", "get",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryGet(c, p); });
    registry.RegisterOperation("assets", "search",
        [this](EditorCore& c, const nlohmann::json& p) { return QuerySearch(c, p); });
    registry.RegisterOperation("assets", "folder_tree",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryFolderTree(c, p); });
    registry.RegisterOperation("assets", "usages",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryUsages(c, p); });
}

nlohmann::json McpAssetsSystem::QueryList(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}

nlohmann::json McpAssetsSystem::QueryGet(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}

nlohmann::json McpAssetsSystem::QuerySearch(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}

nlohmann::json McpAssetsSystem::QueryFolderTree(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}

nlohmann::json McpAssetsSystem::QueryUsages(EditorCore&, const nlohmann::json&)
{
    return {{"ok", false}, {"error", "not yet implemented"}};
}
