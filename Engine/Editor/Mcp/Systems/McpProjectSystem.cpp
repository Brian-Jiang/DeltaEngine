#include "McpProjectSystem.h"

#include "Editor/EditorCore.h"
#include "Editor/Assets/EditorAssetDatabase.h"
#include "Mcp/McpRegistry.h"
#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Core/UUID.h"
#include "Runtime/IO/IOManager.h"
#include "Runtime/Settings/EngineSettings.h"

#include <filesystem>

using namespace DeltaEngine;

static nlohmann::json MakeError(const std::string& msg)
{
    return { {"ok", false}, {"error", msg} };
}

void McpProjectSystem::RegisterTools(McpRegistry& registry)
{
    registry.RegisterQuery("project", "info",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryInfo(c, p); });
    registry.RegisterQuery("project", "settings",
        [this](EditorCore& c, const nlohmann::json& p) { return QuerySettings(c, p); });
    registry.RegisterQuery("project", "open_scenes",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryOpenScenes(c, p); });
    registry.RegisterQuery("project", "build_state",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryBuildState(c, p); });

    registry.RegisterCommand("project", "LoadScene",
        [this](EditorCore& c, const nlohmann::json& p) { return CommandLoadScene(c, p); });
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
    const EngineSettings settings = LoadEngineSettings();
    return {
        {"ok", true},
        {"settings", EngineSettingsToJson(settings)},
        {"settingsFilePath", IOManager::GetEngineSettingsPath().string()}
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

nlohmann::json McpProjectSystem::CommandLoadScene(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("asset_id"))
        return MakeError("missing required param: asset_id");

    const std::string assetIdStr = params["asset_id"].get<std::string>();
    const AssetId assetId = UUID::FromString(assetIdStr);
    if (assetId.IsNull())
        return MakeError("invalid asset_id: " + assetIdStr);

    EditorAssetDatabase* db = core.GetAssetDatabase();
    if (!db)
        return MakeError("no asset database");

    const std::filesystem::path scenePath = db->GetAssetPath(assetId);
    if (scenePath.empty())
        return MakeError("no scene asset registered for asset_id: " + assetIdStr);

    // Reuse the ready LoadScene auxiliary (keyed by path) that the scene system drives.
    nlohmann::json envelope;
    envelope["type"]      = "auxiliary";
    envelope["name"]      = "LoadScene";
    envelope["scenePath"] = scenePath.string();

    core.EnqueueSerializedCommand(envelope.dump());
    return { {"ok", true}, {"queued", true}, {"command", "LoadScene"}, {"expects_result", false} };
}
