#pragma once

#include "Runtime/EngineIncludes.h"

#include "Runtime/Graphics/RenderPath.h"

#include <cstdint>
#include <filesystem>
#include <string_view>

#include <nlohmann/json.hpp>

DELTA_ENGINE_NS_BEGIN

struct ShadowAtlasSettings
{
    uint32_t atlasSize = 4096;
    uint32_t directionalTileSize = 2048;
    uint32_t spotTileSize = 1024;
    uint32_t pointFaceSize = 512;
    uint32_t pointCubeCount = 8;
};

struct GraphicsSettings
{
    RenderPath renderPath = RenderPath::Deferred;
    bool vsync = false;
    ShadowAtlasSettings shadowAtlas;
};

/// Persistent engine configuration (Settings/EngineSettings.json).
/// MCP project.settings returns this same JSON shape via EngineSettingsToJson().
struct EngineSettings
{
    uint32_t version = 1;
    GraphicsSettings graphics;
};

DELTAENGINE_API RenderPath RenderPathFromString(std::string_view value,
    RenderPath fallback = RenderPath::Deferred);
DELTAENGINE_API std::string_view RenderPathToString(RenderPath path);

DELTAENGINE_API EngineSettings GetDefaultEngineSettings();

DELTAENGINE_API nlohmann::json EngineSettingsToJson(const EngineSettings& settings);

DELTAENGINE_API void SaveEngineSettingsToPath(const EngineSettings& settings,
    const std::filesystem::path& path);

DELTAENGINE_API bool LoadEngineSettingsFromPath(EngineSettings& settings,
    const std::filesystem::path& path);

DELTAENGINE_API void SaveEngineSettings(const EngineSettings& settings);

/// Loads engine settings from Settings/EngineSettings.json.
/// Returns defaults if the file is missing or malformed.
DELTAENGINE_API EngineSettings LoadEngineSettings();

/// Loads from path; if missing, writes defaults to that path and returns them.
DELTAENGINE_API EngineSettings LoadOrCreateEngineSettingsFromPath(const std::filesystem::path& path);

/// Loads from Settings/EngineSettings.json; creates the file with defaults if missing.
DELTAENGINE_API EngineSettings LoadOrCreateEngineSettings();

DELTA_ENGINE_NS_END
