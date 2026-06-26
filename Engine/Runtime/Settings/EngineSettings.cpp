#include "Runtime/Settings/EngineSettings.h"

#include "Runtime/IO/IOManager.h"

#include <nlohmann/json.hpp>

#include <exception>
#include <fstream>

using namespace DeltaEngine;

namespace
{
void WriteShadowAtlasSettings(nlohmann::json& parent, const ShadowAtlasSettings& settings)
{
    nlohmann::json shadowAtlas;
    shadowAtlas["atlasSize"] = settings.atlasSize;
    shadowAtlas["directionalTileSize"] = settings.directionalTileSize;
    shadowAtlas["spotTileSize"] = settings.spotTileSize;
    shadowAtlas["pointFaceSize"] = settings.pointFaceSize;
    shadowAtlas["pointCubeCount"] = settings.pointCubeCount;
    parent["shadowAtlas"] = std::move(shadowAtlas);
}

void WriteGraphicsSettings(nlohmann::json& root, const GraphicsSettings& settings)
{
    nlohmann::json graphics;
    graphics["renderPath"] = std::string(RenderPathToString(settings.renderPath));
    graphics["vsync"] = settings.vsync;
    WriteShadowAtlasSettings(graphics, settings.shadowAtlas);
    root["graphics"] = std::move(graphics);
}

void MergeShadowAtlasSettings(const nlohmann::json& parent, ShadowAtlasSettings& settings)
{
    if (!parent.contains("shadowAtlas") || !parent["shadowAtlas"].is_object())
        return;

    const nlohmann::json& shadowAtlas = parent["shadowAtlas"];
    if (shadowAtlas.contains("atlasSize") && shadowAtlas["atlasSize"].is_number_unsigned())
        settings.atlasSize = shadowAtlas["atlasSize"].get<uint32_t>();
    if (shadowAtlas.contains("directionalTileSize") && shadowAtlas["directionalTileSize"].is_number_unsigned())
        settings.directionalTileSize = shadowAtlas["directionalTileSize"].get<uint32_t>();
    if (shadowAtlas.contains("spotTileSize") && shadowAtlas["spotTileSize"].is_number_unsigned())
        settings.spotTileSize = shadowAtlas["spotTileSize"].get<uint32_t>();
    if (shadowAtlas.contains("pointFaceSize") && shadowAtlas["pointFaceSize"].is_number_unsigned())
        settings.pointFaceSize = shadowAtlas["pointFaceSize"].get<uint32_t>();
    if (shadowAtlas.contains("pointCubeCount") && shadowAtlas["pointCubeCount"].is_number_unsigned())
        settings.pointCubeCount = shadowAtlas["pointCubeCount"].get<uint32_t>();
}

void MergeGraphicsSettings(const nlohmann::json& root, GraphicsSettings& settings)
{
    if (!root.contains("graphics") || !root["graphics"].is_object())
        return;

    const nlohmann::json& graphics = root["graphics"];
    if (graphics.contains("renderPath") && graphics["renderPath"].is_string())
        settings.renderPath = RenderPathFromString(graphics["renderPath"].get<std::string>());
    if (graphics.contains("vsync") && graphics["vsync"].is_boolean())
        settings.vsync = graphics["vsync"].get<bool>();
    MergeShadowAtlasSettings(graphics, settings.shadowAtlas);
}
}

RenderPath DeltaEngine::RenderPathFromString(std::string_view value, RenderPath fallback)
{
    if (value == "Forward")
        return RenderPath::Forward;
    if (value == "Deferred")
        return RenderPath::Deferred;

    DLOG(LogIO, ELogLevel::Warning,
        "RenderPathFromString: unknown render path '{}'; using fallback",
        std::string(value));
    return fallback;
}

std::string_view DeltaEngine::RenderPathToString(RenderPath path)
{
    switch (path)
    {
    case RenderPath::Forward:
        return "Forward";
    case RenderPath::Deferred:
        return "Deferred";
    }
    return "Deferred";
}

EngineSettings DeltaEngine::GetDefaultEngineSettings()
{
    return EngineSettings {};
}

void DeltaEngine::SaveEngineSettingsToPath(const EngineSettings& settings,
    const std::filesystem::path& path)
{
    std::filesystem::create_directories(path.parent_path());

    nlohmann::json root;
    root["version"] = settings.version;
    WriteGraphicsSettings(root, settings.graphics);

    std::ofstream file(path);
    if (!file.is_open())
    {
        DLOG(LogIO, ELogLevel::Error,
            "SaveEngineSettingsToPath failed: could not open '{}' for write",
            path.string());
        return;
    }
    file << root.dump(2);
    if (!file.good())
    {
        DLOG(LogIO, ELogLevel::Error,
            "SaveEngineSettingsToPath failed: incomplete write to '{}'",
            path.string());
    }
}

bool DeltaEngine::LoadEngineSettingsFromPath(EngineSettings& settings,
    const std::filesystem::path& path)
{
    if (!std::filesystem::exists(path))
        return false;

    try
    {
        std::ifstream file(path);
        const nlohmann::json root = nlohmann::json::parse(file);
        if (!root.is_object())
        {
            DLOG(LogIO, ELogLevel::Error,
                "LoadEngineSettingsFromPath failed: root is not a JSON object in '{}'",
                path.string());
            return false;
        }

        EngineSettings parsed = GetDefaultEngineSettings();
        if (root.contains("version") && root["version"].is_number_unsigned())
            parsed.version = root["version"].get<uint32_t>();
        MergeGraphicsSettings(root, parsed.graphics);

        settings = std::move(parsed);
        return true;
    }
    catch (const std::exception& ex)
    {
        DLOG(LogIO, ELogLevel::Error,
            "LoadEngineSettingsFromPath failed parsing '{}': {}",
            path.string(), ex.what());
        return false;
    }
}

void DeltaEngine::SaveEngineSettings(const EngineSettings& settings)
{
    SaveEngineSettingsToPath(settings, IOManager::GetEngineSettingsPath());
}

EngineSettings DeltaEngine::LoadEngineSettings()
{
    EngineSettings settings = GetDefaultEngineSettings();
    LoadEngineSettingsFromPath(settings, IOManager::GetEngineSettingsPath());
    return settings;
}

EngineSettings DeltaEngine::LoadOrCreateEngineSettingsFromPath(const std::filesystem::path& path)
{
    if (!std::filesystem::exists(path))
    {
        const EngineSettings defaults = GetDefaultEngineSettings();
        SaveEngineSettingsToPath(defaults, path);
        return defaults;
    }

    EngineSettings settings = GetDefaultEngineSettings();
    LoadEngineSettingsFromPath(settings, path);
    return settings;
}

EngineSettings DeltaEngine::LoadOrCreateEngineSettings()
{
    return LoadOrCreateEngineSettingsFromPath(IOManager::GetEngineSettingsPath());
}
