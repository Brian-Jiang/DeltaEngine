#include "Runtime/Settings/EngineSettings.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

using namespace DeltaEngine;

namespace
{
std::filesystem::path MakeTempRoot()
{
    const auto root =
        std::filesystem::temp_directory_path() /
        ("DeltaEngineSettings_" +
            std::to_string(static_cast<long long>(
                std::chrono::steady_clock::now().time_since_epoch().count())));
    std::filesystem::create_directories(root);
    return root;
}

bool SettingsEqual(const EngineSettings& a, const EngineSettings& b)
{
    return a.version == b.version
        && a.graphics.renderPath == b.graphics.renderPath
        && a.graphics.vsync == b.graphics.vsync
        && a.graphics.shadowAtlas.atlasSize == b.graphics.shadowAtlas.atlasSize
        && a.graphics.shadowAtlas.directionalTileSize == b.graphics.shadowAtlas.directionalTileSize
        && a.graphics.shadowAtlas.spotTileSize == b.graphics.shadowAtlas.spotTileSize
        && a.graphics.shadowAtlas.pointFaceSize == b.graphics.shadowAtlas.pointFaceSize
        && a.graphics.shadowAtlas.pointCubeCount == b.graphics.shadowAtlas.pointCubeCount;
}
}

TEST(EngineSettingsTests, SaveAndLoad_RoundTrip_PreservesAllFields)
{
    const auto root = MakeTempRoot();
    const auto path = root / "EngineSettings.json";

    const EngineSettings in = GetDefaultEngineSettings();

    SaveEngineSettingsToPath(in, path);
    ASSERT_TRUE(std::filesystem::exists(path));

    EngineSettings out;
    ASSERT_TRUE(LoadEngineSettingsFromPath(out, path));
    EXPECT_TRUE(SettingsEqual(out, in));

    std::error_code ec;
    std::filesystem::remove_all(root, ec);
}

TEST(EngineSettingsTests, Load_MissingFile_ReturnsFalseAndLeavesStateUnchanged)
{
    const auto root = MakeTempRoot();
    const auto path = root / "does_not_exist.json";

    EngineSettings settings;
    settings.version = 99;
    settings.graphics.renderPath = RenderPath::Forward;
    settings.graphics.vsync = true;
    settings.graphics.shadowAtlas.atlasSize = 1;

    EXPECT_FALSE(LoadEngineSettingsFromPath(settings, path));
    EXPECT_EQ(settings.version, 99u);
    EXPECT_EQ(settings.graphics.renderPath, RenderPath::Forward);
    EXPECT_TRUE(settings.graphics.vsync);
    EXPECT_EQ(settings.graphics.shadowAtlas.atlasSize, 1u);

    std::error_code ec;
    std::filesystem::remove_all(root, ec);
}

TEST(EngineSettingsTests, Load_MalformedJson_ReturnsFalseAndLeavesStateUnchanged)
{
    const auto root = MakeTempRoot();
    const auto path = root / "EngineSettings.json";
    {
        std::ofstream out(path);
        out << "not valid json {{{";
    }

    EngineSettings settings;
    settings.version = 99;
    settings.graphics.renderPath = RenderPath::Forward;
    settings.graphics.vsync = true;

    EXPECT_FALSE(LoadEngineSettingsFromPath(settings, path));
    EXPECT_EQ(settings.version, 99u);
    EXPECT_EQ(settings.graphics.renderPath, RenderPath::Forward);
    EXPECT_TRUE(settings.graphics.vsync);

    std::error_code ec;
    std::filesystem::remove_all(root, ec);
}

TEST(EngineSettingsTests, Load_UnknownRenderPath_FallsBackToDeferred)
{
    const auto root = MakeTempRoot();
    const auto path = root / "EngineSettings.json";
    {
        std::ofstream out(path);
        out << R"({
  "version": 1,
  "graphics": {
    "renderPath": "RayTraced"
  }
})";
    }

    EngineSettings settings;
    ASSERT_TRUE(LoadEngineSettingsFromPath(settings, path));
    EXPECT_EQ(settings.graphics.renderPath, RenderPath::Deferred);

    std::error_code ec;
    std::filesystem::remove_all(root, ec);
}

TEST(EngineSettingsTests, Load_PartialGraphicsObject_MergesDefaultsForMissingKeys)
{
    const auto root = MakeTempRoot();
    const auto path = root / "EngineSettings.json";
    {
        std::ofstream out(path);
        out << R"({
  "version": 1,
  "graphics": {
    "vsync": true
  }
})";
    }

    EngineSettings settings;
    ASSERT_TRUE(LoadEngineSettingsFromPath(settings, path));

    const EngineSettings defaults = GetDefaultEngineSettings();
    EXPECT_EQ(settings.graphics.renderPath, RenderPath::Deferred);
    EXPECT_TRUE(settings.graphics.vsync);
    EXPECT_EQ(settings.graphics.shadowAtlas.atlasSize, defaults.graphics.shadowAtlas.atlasSize);
    EXPECT_EQ(settings.graphics.shadowAtlas.directionalTileSize, defaults.graphics.shadowAtlas.directionalTileSize);
    EXPECT_EQ(settings.graphics.shadowAtlas.spotTileSize, defaults.graphics.shadowAtlas.spotTileSize);
    EXPECT_EQ(settings.graphics.shadowAtlas.pointFaceSize, defaults.graphics.shadowAtlas.pointFaceSize);
    EXPECT_EQ(settings.graphics.shadowAtlas.pointCubeCount, defaults.graphics.shadowAtlas.pointCubeCount);

    std::error_code ec;
    std::filesystem::remove_all(root, ec);
}

TEST(EngineSettingsTests, LoadEngineSettings_MissingFile_ReturnsDefaults)
{
    const auto root = MakeTempRoot();
    const auto path = root / "does_not_exist.json";

    EngineSettings settings = GetDefaultEngineSettings();
    EXPECT_FALSE(LoadEngineSettingsFromPath(settings, path));
    EXPECT_TRUE(SettingsEqual(settings, GetDefaultEngineSettings()));

    std::error_code ec;
    std::filesystem::remove_all(root, ec);
}

TEST(EngineSettingsTests, RenderPathFromString_AndToString_RoundTrip)
{
    EXPECT_EQ(RenderPathFromString("Forward"), RenderPath::Forward);
    EXPECT_EQ(RenderPathFromString("Deferred"), RenderPath::Deferred);
    EXPECT_EQ(RenderPathToString(RenderPath::Forward), "Forward");
    EXPECT_EQ(RenderPathToString(RenderPath::Deferred), "Deferred");
}

TEST(EngineSettingsTests, LoadOrCreateEngineSettingsFromPath_MissingFile_CreatesDefaultsOnDisk)
{
    const auto root = MakeTempRoot();
    const auto path = root / "EngineSettings.json";

    ASSERT_FALSE(std::filesystem::exists(path));

    const EngineSettings settings = LoadOrCreateEngineSettingsFromPath(path);
    EXPECT_TRUE(SettingsEqual(settings, GetDefaultEngineSettings()));
    ASSERT_TRUE(std::filesystem::exists(path));

    EngineSettings loaded;
    ASSERT_TRUE(LoadEngineSettingsFromPath(loaded, path));
    EXPECT_TRUE(SettingsEqual(loaded, GetDefaultEngineSettings()));

    std::error_code ec;
    std::filesystem::remove_all(root, ec);
}

TEST(EngineSettingsTests, EngineSettingsToJson_MatchesSaveFormat)
{
    const auto root = MakeTempRoot();
    const auto path = root / "EngineSettings.json";

    const EngineSettings in = GetDefaultEngineSettings();
    SaveEngineSettingsToPath(in, path);
    ASSERT_TRUE(std::filesystem::exists(path));

    EngineSettings loaded;
    ASSERT_TRUE(LoadEngineSettingsFromPath(loaded, path));
    EXPECT_EQ(EngineSettingsToJson(loaded), EngineSettingsToJson(in));

    std::error_code ec;
    std::filesystem::remove_all(root, ec);
}
