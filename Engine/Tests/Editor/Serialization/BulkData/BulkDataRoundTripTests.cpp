#include "Shared/SerializationTestSupport.h"

#include "Runtime/Test/SerializationTestTypes.h"

#include <fstream>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class BulkDataRoundTripTests : public EditorSerializationTest
{
};

TEST_F(BulkDataRoundTripTests, RoundTripsBulkDataAndWritesSidecars)
{
    const auto tempDir = MakeTempDir("DeltaBulkTest");

    auto* asset = CreateDObject<PA_TestMesh>();
    const AssetId assetId = AssetId::Generate();
    asset->GetHeader().m_persistentId = assetId;
    asset->GetHeader().m_className = "PA_TestMesh";

    auto* mesh = new DTestMeshData();
    const ObjectId meshId = ObjectId::Generate();
    mesh->SetObjectId(meshId);
    mesh->m_vertexCount = 100;
    mesh->m_indexCount = 300;

    std::vector<uint8_t> vertices(400);
    for (int index = 0; index < 400; ++index)
        vertices[index] = static_cast<uint8_t>(index % 256);
    mesh->m_vertexBuffer.Set(vertices.data(), static_cast<uint64_t>(vertices.size()));

    std::vector<uint8_t> indices(600);
    for (int index = 0; index < 600; ++index)
        indices[index] = static_cast<uint8_t>((index * 7) % 256);
    mesh->m_indexBuffer.Set(indices.data(), static_cast<uint64_t>(indices.size()));

    asset->AddObject(mesh);

    const auto filePath = tempDir.Path() / "TestMesh.dasset.json";
    SaveAssetWithBulkData(asset, filePath);

    ASSERT_TRUE(std::filesystem::exists(tempDir.Path() / "TestMesh_Bulk0.bin"));
    ASSERT_TRUE(std::filesystem::exists(tempDir.Path() / "TestMesh_Bulk1.bin"));
    EXPECT_EQ(std::filesystem::file_size(tempDir.Path() / "TestMesh_Bulk0.bin"), 400u);
    EXPECT_EQ(std::filesystem::file_size(tempDir.Path() / "TestMesh_Bulk1.bin"), 600u);

    {
        std::ifstream input(filePath);
        const auto json = nlohmann::json::parse(input);
        EXPECT_EQ(json["objects"][0]["m_vertexBuffer"]["_bulk"], 0);
        EXPECT_EQ(json["objects"][0]["m_vertexBuffer"]["size"], 400);
        EXPECT_EQ(json["objects"][0]["m_indexBuffer"]["_bulk"], 1);
        EXPECT_EQ(json["objects"][0]["m_indexBuffer"]["size"], 600);
        EXPECT_EQ(json["header"]["bulkDataMap"]["0"]["size"], 400);
        EXPECT_EQ(json["header"]["bulkDataMap"]["1"]["size"], 600);
    }

    EditorAssetDatabase database;
    database.ScanAssetsFolder(tempDir.Path());
    DPrimaryAsset* loaded = database.LoadAsset(assetId);

    ASSERT_NE(loaded, nullptr);
    auto* loadedMesh = FindObjectAs<DTestMeshData>(loaded, meshId);
    ASSERT_NE(loadedMesh, nullptr);
    EXPECT_EQ(loadedMesh->m_vertexCount, 100);
    EXPECT_EQ(loadedMesh->m_indexCount, 300);
    ASSERT_EQ(loadedMesh->m_vertexBuffer.m_size, 400u);
    for (int index = 0; index < 400; ++index)
        EXPECT_EQ(loadedMesh->m_vertexBuffer.m_data[index], static_cast<uint8_t>(index % 256));

    ASSERT_EQ(loadedMesh->m_indexBuffer.m_size, 600u);
    for (int index = 0; index < 600; ++index)
        EXPECT_EQ(loadedMesh->m_indexBuffer.m_data[index], static_cast<uint8_t>((index * 7) % 256));
}

TEST_F(BulkDataRoundTripTests, AssetEnumerationIgnoresBulkSidecars)
{
    const auto tempDir = MakeTempDir("DeltaAssetEnumerationTest");

    const AssetId assetId = AssetId::Generate();
    const nlohmann::json file = {
        {"header", {
            {"magic", "DLTA"},
            {"version", 1},
            {"className", "PA_TestAsset"},
            {"assetId", assetId.ToString()}
        }},
        {"objects", nlohmann::json::array()}
    };

    SaveJsonToFile(file, tempDir.Path() / "Enumerated.dasset.json");
    {
        std::ofstream bulkFile(tempDir.Path() / "Enumerated_Bulk0.bin", std::ios::binary);
        bulkFile << "bulk";
    }

    EditorAssetDatabase database;
    database.ScanAssetsFolder(tempDir.Path());

    const auto& assets = database.GetAllAssets();
    ASSERT_EQ(assets.size(), 1u);
    EXPECT_EQ(assets.begin()->first, assetId);
    EXPECT_EQ(assets.begin()->second.m_filePath.filename(), "Enumerated.dasset.json");
}
