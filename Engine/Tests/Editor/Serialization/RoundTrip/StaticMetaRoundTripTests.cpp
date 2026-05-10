#include "Shared/SerializationTestSupport.h"

#include "Runtime/Assets/PA_CommonAssets.h"
#include "Runtime/Core/DMesh.h"
#include "Runtime/IO/IOManager.h"

#include <fstream>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

namespace
{
std::filesystem::path SphereFbxPath()
{
    return IOManager::GetEngineSourceAssetFullPath(std::filesystem::path("sphere.fbx"));
}

PA_StaticMesh* MakeImportedSphereAsset()
{
    DMesh* mesh = CreateDObject<DMesh>();
    mesh->ImportFromAbsolutePath(SphereFbxPath());
    return PA_StaticMesh::Create(mesh);
}

nlohmann::json ReadJson(const std::filesystem::path& path)
{
    std::ifstream in(path);
    nlohmann::json j;
    in >> j;
    return j;
}
}

class StaticMetaRoundTripTests : public EditorSerializationTest
{
};

TEST_F(StaticMetaRoundTripTests, CreatePopulatesStaticMetaFromImportedMesh)
{
    PA_StaticMesh* asset = MakeImportedSphereAsset();
    ASSERT_NE(asset, nullptr);
    ASSERT_NE(asset->GetStaticMesh(), nullptr);

    const auto& meta = asset->GetStaticMeta();
    EXPECT_GT(meta.m_vertexCount, 0);
    EXPECT_GT(meta.m_indexCount, 0);
    const float extentLen =
        meta.m_aabb.Extents.x + meta.m_aabb.Extents.y + meta.m_aabb.Extents.z;
    EXPECT_GT(extentLen, 0.f);
}

TEST_F(StaticMetaRoundTripTests, RoundTripsThroughDisk)
{
    const auto tempDir = MakeTempDir("DeltaStaticMetaRoundTrip");
    PA_StaticMesh* asset = MakeImportedSphereAsset();
    const AssetId assetId = asset->GetAssetId();
    const PA_StaticMesh_StaticMeta beforeSave = asset->GetStaticMeta();

    const auto filePath = tempDir.Path() / "Sphere.dasset.json";
    SaveAssetWithBulkData(asset, filePath);

    EditorAssetDatabase database;
    database.ScanAssetsFolder(tempDir.Path());
    auto* loaded = dynamic_cast<PA_StaticMesh*>(database.LoadAsset(assetId));
    ASSERT_NE(loaded, nullptr);

    const auto& after = loaded->GetStaticMeta();
    EXPECT_EQ(after.m_vertexCount, beforeSave.m_vertexCount);
    EXPECT_EQ(after.m_indexCount, beforeSave.m_indexCount);
    EXPECT_FLOAT_EQ(after.m_aabb.Center.x,  beforeSave.m_aabb.Center.x);
    EXPECT_FLOAT_EQ(after.m_aabb.Center.y,  beforeSave.m_aabb.Center.y);
    EXPECT_FLOAT_EQ(after.m_aabb.Center.z,  beforeSave.m_aabb.Center.z);
    EXPECT_FLOAT_EQ(after.m_aabb.Extents.x, beforeSave.m_aabb.Extents.x);
    EXPECT_FLOAT_EQ(after.m_aabb.Extents.y, beforeSave.m_aabb.Extents.y);
    EXPECT_FLOAT_EQ(after.m_aabb.Extents.z, beforeSave.m_aabb.Extents.z);
}

TEST_F(StaticMetaRoundTripTests, StaticMetaLivesAtMetaStaticNotInObjects)
{
    const auto tempDir = MakeTempDir("DeltaStaticMetaShape");
    PA_StaticMesh* asset = MakeImportedSphereAsset();

    const auto filePath = tempDir.Path() / "Shape.dasset.json";
    SaveAssetWithBulkData(asset, filePath);

    nlohmann::json onDisk = ReadJson(filePath);
    ASSERT_TRUE(onDisk.contains("meta"));
    ASSERT_TRUE(onDisk["meta"].contains("static"));
    const auto& staticBlock = onDisk["meta"]["static"];
    ASSERT_TRUE(staticBlock.is_object());
    EXPECT_TRUE(staticBlock.contains("m_vertexCount"));
    EXPECT_TRUE(staticBlock.contains("m_indexCount"));
    EXPECT_TRUE(staticBlock.contains("m_aabb"));

    ASSERT_TRUE(onDisk.contains("objects"));
    ASSERT_TRUE(onDisk["objects"].is_array());
    for (const auto& obj : onDisk["objects"])
    {
        EXPECT_FALSE(obj.contains("m_vertexCount"));
        EXPECT_FALSE(obj.contains("m_indexCount"));
        EXPECT_FALSE(obj.contains("m_aabb"));
    }
}

TEST_F(StaticMetaRoundTripTests, RebuildStaticMetaRecomputesFromMesh)
{
    DMesh* emptyMesh = CreateDObject<DMesh>();
    PA_StaticMesh* asset = PA_StaticMesh::Create(emptyMesh);
    ASSERT_NE(asset, nullptr);
    EXPECT_EQ(asset->GetStaticMeta().m_vertexCount, 0);
    EXPECT_EQ(asset->GetStaticMeta().m_indexCount, 0);

    emptyMesh->ImportFromAbsolutePath(SphereFbxPath());
    asset->RebuildStaticMeta();

    PA_StaticMesh* sibling = MakeImportedSphereAsset();
    ASSERT_NE(sibling, nullptr);

    const auto& got = asset->GetStaticMeta();
    const auto& expected = sibling->GetStaticMeta();
    EXPECT_GT(got.m_vertexCount, 0);
    EXPECT_GT(got.m_indexCount, 0);
    EXPECT_EQ(got.m_vertexCount, expected.m_vertexCount);
    EXPECT_EQ(got.m_indexCount, expected.m_indexCount);
    EXPECT_FLOAT_EQ(got.m_aabb.Extents.x, expected.m_aabb.Extents.x);
    EXPECT_FLOAT_EQ(got.m_aabb.Extents.y, expected.m_aabb.Extents.y);
    EXPECT_FLOAT_EQ(got.m_aabb.Extents.z, expected.m_aabb.Extents.z);
}
