#include "Runtime/Serialization/JsonAssetArchive.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

using namespace DeltaEngine;

namespace
{
class ScopedSerializationTempDir
{
public:
    explicit ScopedSerializationTempDir(const std::string& name)
        : m_path(std::filesystem::temp_directory_path() / name)
    {
        std::error_code errorCode;
        std::filesystem::remove_all(m_path, errorCode);
        std::filesystem::create_directories(m_path);
    }

    ~ScopedSerializationTempDir()
    {
        std::error_code errorCode;
        std::filesystem::remove_all(m_path, errorCode);
    }

    const std::filesystem::path& Path() const
    {
        return m_path;
    }

private:
    std::filesystem::path m_path;
};
}

TEST(JsonAssetArchiveHardeningTests, JsonAssetArchive_WriteBulkData_DoesNotPublishMapEntryWhenDirectoryIsFile)
{
    const ScopedSerializationTempDir tempDir("DeltaJsonArchiveDirectoryFile");
    const std::filesystem::path assetDirFile = tempDir.Path() / "NotADirectory";
    {
        std::ofstream output(assetDirFile);
        output << "not a directory";
    }
    const std::array<uint8_t, 3> payload{ 1, 2, 3 };
    JsonAssetArchive archive(assetDirFile, "Asset");

    archive.WriteBulkData(0, payload.data(), payload.size());

    EXPECT_FALSE(archive.GetRoot().contains("header"));
    EXPECT_FALSE(std::filesystem::exists(assetDirFile / "Asset_Bulk0.bin"));
}

TEST(JsonAssetArchiveHardeningTests, JsonAssetArchive_ReadBulkData_ReturnsEmptyWhenSidecarSizeDoesNotMatch)
{
    const ScopedSerializationTempDir tempDir("DeltaJsonArchiveSidecarMismatch");
    const std::filesystem::path sidecarPath = tempDir.Path() / "Asset_Bulk0.bin";
    {
        std::ofstream output(sidecarPath, std::ios::binary);
        const std::array<uint8_t, 2> bytes{ 4, 5 };
        output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }
    const nlohmann::json root = {
        { "header", {
            { "bulkDataMap", {
                { "0", {
                    { "file", "Asset_Bulk0.bin" },
                    { "size", 4 }
                } }
            } }
        } }
    };
    JsonAssetArchive archive(root, tempDir.Path());

    // Regression: sidecar size mismatches used to return truncated payload bytes silently.
    const std::vector<uint8_t> bytes = archive.ReadBulkData(0);

    EXPECT_TRUE(bytes.empty());
}

TEST(JsonAssetArchiveHardeningTests, JsonAssetArchive_ReadBulkData_RejectsPathTraversalSidecar)
{
    const ScopedSerializationTempDir tempDir("DeltaJsonArchiveTraversal");
    const nlohmann::json root = {
        { "header", {
            { "bulkDataMap", {
                { "0", {
                    { "file", "../Outside.bin" },
                    { "size", 1 }
                } }
            } }
        } }
    };
    JsonAssetArchive archive(root, tempDir.Path());

    const std::vector<uint8_t> bytes = archive.ReadBulkData(0);

    EXPECT_TRUE(bytes.empty());
}

TEST(JsonAssetArchiveHardeningTests, JsonAssetArchive_ReadBulkData_RejectsDriveRelativeSidecarPath)
{
    const ScopedSerializationTempDir tempDir("DeltaJsonArchiveDriveRelative");
    const nlohmann::json root = {
        { "header", {
            { "bulkDataMap", {
                { "0", {
                    { "file", "C:Outside.bin" },
                    { "size", 1 }
                } }
            } }
        } }
    };
    JsonAssetArchive archive(root, tempDir.Path());

    const std::vector<uint8_t> bytes = archive.ReadBulkData(0);

    EXPECT_TRUE(bytes.empty());
}

TEST(JsonAssetArchiveHardeningTests, JsonAssetArchive_WriteBulkData_RejectsUnsafeAssetName)
{
    const ScopedSerializationTempDir tempDir("DeltaJsonArchiveUnsafeAssetName");
    const std::filesystem::path assetDir = tempDir.Path() / "AssetDir";
    std::filesystem::create_directories(assetDir);
    const std::array<uint8_t, 1> payload{ 9 };
    JsonAssetArchive archive(assetDir, "../EscapedAsset");

    archive.WriteBulkData(0, payload.data(), payload.size());

    EXPECT_FALSE(archive.GetRoot().contains("header"));
    EXPECT_FALSE(std::filesystem::exists(tempDir.Path() / "EscapedAsset_Bulk0.bin"));
}

TEST(JsonAssetArchiveHardeningTests, JsonAssetArchive_ReadBulkData_ReadsSidecarFromPathWithSpacesAndUnicode)
{
    const ScopedSerializationTempDir tempDir("Delta Json Archive Unicode");
    const std::filesystem::path assetDir = tempDir.Path() / u8"Sub Dir Cafe \u00E9";
    std::filesystem::create_directories(assetDir);
    const std::filesystem::path sidecarPath = assetDir / "Asset_Bulk0.bin";
    {
        std::ofstream output(sidecarPath, std::ios::binary);
        const std::array<uint8_t, 3> bytes{ 7, 8, 9 };
        output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }
    const nlohmann::json root = {
        { "header", {
            { "bulkDataMap", {
                { "0", {
                    { "file", "Asset_Bulk0.bin" },
                    { "size", 3 }
                } }
            } }
        } }
    };
    JsonAssetArchive archive(root, assetDir);

    const std::vector<uint8_t> bytes = archive.ReadBulkData(0);

    ASSERT_EQ(bytes.size(), 3u);
    EXPECT_EQ(bytes[0], 7u);
    EXPECT_EQ(bytes[1], 8u);
    EXPECT_EQ(bytes[2], 9u);
}

TEST(JsonAssetArchiveHardeningTests, JsonAssetArchive_SerializeMalformedScalar_KeepsExistingValue)
{
    const nlohmann::json root = { { "value", "not an int" } };
    JsonAssetArchive archive(root, {});
    int value = 42;

    archive.Serialize("value", value);

    EXPECT_EQ(value, 42);
}

TEST(JsonAssetArchiveHardeningTests, JsonAssetArchive_BeginArrayLoadWithNonArray_ReturnsZeroAndKeepsStackBalanced)
{
    const nlohmann::json root = {
        { "items", 5 },
        { "after", 9 }
    };
    JsonAssetArchive archive(root, {});

    const size_t count = archive.BeginArrayLoad("items");
    archive.EndArray();
    int after = 0;
    archive.Serialize("after", after);

    EXPECT_EQ(count, 0u);
    EXPECT_EQ(after, 9);
}

TEST(JsonAssetArchiveHardeningTests, JsonAssetArchive_BeginObjectLoadWithoutClass_ReturnsEmptyClassName)
{
    const nlohmann::json root = { { "value", 5 } };
    JsonAssetArchive archive(root, {});

    const std::string className = archive.BeginObjectLoad();
    archive.EndObject();

    EXPECT_TRUE(className.empty());
}
