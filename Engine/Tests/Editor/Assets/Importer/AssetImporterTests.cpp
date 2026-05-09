#include "Shared/SerializationTestSupport.h"

#include "Editor/Assets/AssetImporter.h"
#include "Editor/Assets/EditorAssetDatabase.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

namespace
{
class AssetImporterTests : public EditorSerializationTest
{
};

void WriteEmptyFile(const std::filesystem::path& path)
{
    std::ofstream out(path, std::ios::binary);
    out << "stub";
}
}

TEST_F(AssetImporterTests, ImportFile_EmptyPath_ReturnsEmpty)
{
    EditorAssetDatabase db;

    const auto ids = AssetImporter::ImportFile({}, db);

    EXPECT_TRUE(ids.empty());
}

TEST_F(AssetImporterTests, ImportFile_NonExistentPath_ReturnsEmpty)
{
    const auto tempDir = MakeTempDir("DeltaImporter_Missing");
    EditorAssetDatabase db;

    const auto ids = AssetImporter::ImportFile(tempDir.Path() / "does_not_exist.png", db);

    EXPECT_TRUE(ids.empty());
}

TEST_F(AssetImporterTests, ImportFile_UnsupportedExtension_ReturnsEmpty)
{
    const auto tempDir = MakeTempDir("DeltaImporter_Unsupported");
    const auto txt = tempDir.Path() / "thing.txt";
    WriteEmptyFile(txt);

    EditorAssetDatabase db;
    const auto ids = AssetImporter::ImportFile(txt, db);

    EXPECT_TRUE(ids.empty());
}

