#include "McpProjectSystem.h"

#include "Editor/EditorCore.h"
#include "Editor/Assets/EditorAssetDatabase.h"
#include "Mcp/McpRegistry.h"
#include "Runtime/Assets/DPrimaryAsset.h"

#include <filesystem>

using namespace DeltaEngine;

static nlohmann::json MakeError(const std::string& msg)
{
    return { {"ok", false}, {"error", msg} };
}

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
    std::string rootPath = std::filesystem::weakly_canonical(
        std::filesystem::current_path() / "../../../").string();

    return {
        {"ok", true},
        {"project", {
            {"name", "DefaultProject"},
            {"root_path", rootPath},
            {"engine_version", "0.1.0-dev"},
            {"configuration", "x64-Debug"}
        }}
    };
}

nlohmann::json McpProjectSystem::QuerySettings(EditorCore&, const nlohmann::json&)
{
    return {
        {"ok", true},
        {"settings", nlohmann::json::object()}
    };
}

nlohmann::json McpProjectSystem::QueryOpenScenes(EditorCore& core, const nlohmann::json&)
{
    nlohmann::json arr = nlohmann::json::array();

    DPrimaryAsset* active = core.GetActiveSceneAsset();
    if (active)
    {
        const auto& header = active->GetHeader();
        nlohmann::json entry;
        entry["asset_id"] = header.m_persistentId.ToString();
        entry["class"] = header.m_className;
        entry["is_dirty"] = active->IsDirty();

        EditorAssetDatabase* db = core.GetAssetDatabase();
        if (db)
        {
            std::filesystem::path path = db->GetAssetPath(header.m_persistentId);
            if (!path.empty())
                entry["file_path"] = path.string();
        }

        arr.push_back(std::move(entry));
    }

    return { {"ok", true}, {"scenes", std::move(arr)} };
}

nlohmann::json McpProjectSystem::QueryBuildState(EditorCore&, const nlohmann::json&)
{
    return {
        {"ok", true},
        {"build", {
            {"configuration", "x64-Debug"},
            {"target_platform", "Windows"},
            {"status", "Default"}
        }}
    };
}
