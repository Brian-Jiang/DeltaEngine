#include "Runtime/IO/IOManager.h"

#include "Runtime/Utils/StringUtils.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

using namespace DeltaEngine;

TEST(IOManager, IOManager_GetProjectRoot_ReturnsExistingAbsoluteDirectory)
{
    const std::filesystem::path& root = IOManager::GetProjectRoot();

    EXPECT_FALSE(root.empty());
    EXPECT_TRUE(root.is_absolute());
    EXPECT_TRUE(std::filesystem::exists(root));
    EXPECT_TRUE(std::filesystem::is_directory(root));
}

TEST(IOManager, IOManager_GetProjectRoot_IsIdempotent)
{
    const std::filesystem::path a = IOManager::GetProjectRoot();
    const std::filesystem::path b = IOManager::GetProjectRoot();

    EXPECT_EQ(a, b);
}

TEST(IOManager, IOManager_GetIntermediateFolder_ResolvesUnderProjectRoot)
{
    const std::filesystem::path root = IOManager::GetProjectRoot();

    const std::filesystem::path inter = IOManager::GetIntermediateFolder();

    EXPECT_TRUE(inter.is_absolute());
    EXPECT_EQ(inter.parent_path(), root);
    EXPECT_EQ(inter.filename(), "Intermediate");
}

TEST(IOManager, IOManager_GetToolsFolder_ResolvesUnderProjectRoot)
{
    const std::filesystem::path tools = IOManager::GetToolsFolder();

    EXPECT_EQ(tools.parent_path(), IOManager::GetProjectRoot());
    EXPECT_EQ(tools.filename(), "Tools");
}

TEST(IOManager, IOManager_GetSettingsFolder_ResolvesUnderProjectRoot)
{
    const std::filesystem::path settings = IOManager::GetSettingsFolder();

    EXPECT_EQ(settings.parent_path(), IOManager::GetProjectRoot());
    EXPECT_EQ(settings.filename(), "Settings");
}

TEST(IOManager, IOManager_GetEngineSettingsPath_ResolvesUnderSettingsFolder)
{
    const std::filesystem::path path = IOManager::GetEngineSettingsPath();

    EXPECT_EQ(path.parent_path(), IOManager::GetSettingsFolder());
    EXPECT_EQ(path.filename(), "EngineSettings.json");
}

TEST(IOManager, IOManager_GetEngineImportedAssetFullPath_AppendsJsonExtension_WhenIsJsonTrue)
{
    const std::filesystem::path full =
        IOManager::GetEngineImportedAssetFullPath("MyAsset", true);

    EXPECT_EQ(full.filename().string(), "MyAsset.dasset.json");
}

TEST(IOManager, IOManager_GetEngineImportedAssetFullPath_AppendsBinaryExtension_WhenIsJsonFalse)
{
    const std::filesystem::path full =
        IOManager::GetEngineImportedAssetFullPath("MyAsset", false);

    EXPECT_EQ(full.filename().string(), "MyAsset.dasset");
}

TEST(IOManager, IOManager_GetEngineSourceAssetFullPath_AcceptsPathWithSpaces)
{
    const std::filesystem::path full =
        IOManager::GetEngineSourceAssetFullPath("Sub Folder/Asset Name.txt");

    EXPECT_NE(full.string().find("Sub Folder"), std::string::npos);
    EXPECT_EQ(full.filename().string(), "Asset Name.txt");
}

TEST(IOManager, IOManager_GetEngineSourceAssetFullPath_RoundTripsUnicodeAssetName)
{
    const std::filesystem::path name(L"Шейдеры/星のテクスチャ.dds");
    const std::filesystem::path full =
        IOManager::GetEngineSourceAssetFullPath(name);

    const std::string expected = StringUtils::WStringToUtf8(L"星のテクスチャ.dds");
    EXPECT_EQ(StringUtils::PathToUtf8(full.filename()), expected);
}

TEST(IOManager, IOManager_GetEngineImportedAssetsFolder_DoesNotDependOnCurrentDirectory)
{
    const std::filesystem::path firstCall = IOManager::GetEngineImportedAssetsFolder();

    const std::filesystem::path savedCwd = std::filesystem::current_path();
    std::filesystem::current_path(std::filesystem::temp_directory_path());
    const std::filesystem::path secondCall = IOManager::GetEngineImportedAssetsFolder();
    std::filesystem::current_path(savedCwd);

    EXPECT_EQ(firstCall, secondCall);
}
