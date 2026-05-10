#include "Shared/SerializationTestSupport.h"

#include "Runtime/Assets/PA_CommonAssets.h"
#include "Runtime/Core/DTexture.h"
#include "Runtime/IO/IOManager.h"

#include <fstream>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

namespace
{
std::filesystem::path SampleTexturePath()
{
    return IOManager::GetEngineSourceAssetFullPath(std::filesystem::path("SkyboxCubemap.dds"));
}

PA_Texture* MakeImportedTextureAsset()
{
    DTexture* texture = CreateDObject<DTexture>();
    texture->Initialize(SampleTexturePath());
    return PA_Texture::Create(texture);
}

nlohmann::json ReadJson(const std::filesystem::path& path)
{
    std::ifstream in(path);
    nlohmann::json j;
    in >> j;
    return j;
}
}

class StaticMetaRoundTripTests_Texture : public EditorSerializationTest
{
};

TEST_F(StaticMetaRoundTripTests_Texture, CreatePopulatesStaticMetaFromImportedTexture)
{
    PA_Texture* asset = MakeImportedTextureAsset();
    ASSERT_NE(asset, nullptr);
    ASSERT_NE(asset->GetTexture(), nullptr);

    const auto& meta = asset->GetStaticMeta();
    EXPECT_GT(meta.m_width, 0);
    EXPECT_GT(meta.m_height, 0);
    EXPECT_GT(meta.m_mipCount, 0);
    EXPECT_NE(meta.m_format, 0);
}

TEST_F(StaticMetaRoundTripTests_Texture, RoundTripsThroughDisk)
{
    const auto tempDir = MakeTempDir("DeltaTextureStaticMetaRoundTrip");
    PA_Texture* asset = MakeImportedTextureAsset();
    const AssetId assetId = asset->GetAssetId();
    const PA_Texture_StaticMeta beforeSave = asset->GetStaticMeta();

    const auto filePath = tempDir.Path() / "Cubemap.dasset.json";
    SaveAssetWithBulkData(asset, filePath);

    EditorAssetDatabase database;
    database.ScanAssetsFolder(tempDir.Path());
    auto* loaded = dynamic_cast<PA_Texture*>(database.LoadAsset(assetId));
    ASSERT_NE(loaded, nullptr);

    const auto& after = loaded->GetStaticMeta();
    EXPECT_EQ(after.m_width, beforeSave.m_width);
    EXPECT_EQ(after.m_height, beforeSave.m_height);
    EXPECT_EQ(after.m_mipCount, beforeSave.m_mipCount);
    EXPECT_EQ(after.m_format, beforeSave.m_format);
}

TEST_F(StaticMetaRoundTripTests_Texture, StaticMetaLivesAtMetaStaticNotInObjects)
{
    const auto tempDir = MakeTempDir("DeltaTextureStaticMetaShape");
    PA_Texture* asset = MakeImportedTextureAsset();

    const auto filePath = tempDir.Path() / "Shape.dasset.json";
    SaveAssetWithBulkData(asset, filePath);

    nlohmann::json onDisk = ReadJson(filePath);
    ASSERT_TRUE(onDisk.contains("meta"));
    ASSERT_TRUE(onDisk["meta"].contains("static"));
    const auto& staticBlock = onDisk["meta"]["static"];
    ASSERT_TRUE(staticBlock.is_object());
    EXPECT_TRUE(staticBlock.contains("m_width"));
    EXPECT_TRUE(staticBlock.contains("m_height"));
    EXPECT_TRUE(staticBlock.contains("m_mipCount"));
    EXPECT_TRUE(staticBlock.contains("m_format"));

    ASSERT_TRUE(onDisk.contains("objects"));
    ASSERT_TRUE(onDisk["objects"].is_array());
    for (const auto& obj : onDisk["objects"])
    {
        EXPECT_FALSE(obj.contains("m_width"));
        EXPECT_FALSE(obj.contains("m_height"));
        EXPECT_FALSE(obj.contains("m_mipCount"));
    }
}

TEST_F(StaticMetaRoundTripTests_Texture, RebuildStaticMetaRecomputesFromTexture)
{
    DTexture* emptyTexture = CreateDObject<DTexture>();
    PA_Texture* asset = PA_Texture::Create(emptyTexture);
    ASSERT_NE(asset, nullptr);
    EXPECT_EQ(asset->GetStaticMeta().m_width, 0);
    EXPECT_EQ(asset->GetStaticMeta().m_height, 0);

    emptyTexture->Initialize(SampleTexturePath());
    asset->RebuildStaticMeta();

    PA_Texture* sibling = MakeImportedTextureAsset();
    ASSERT_NE(sibling, nullptr);

    const auto& got = asset->GetStaticMeta();
    const auto& expected = sibling->GetStaticMeta();
    EXPECT_GT(got.m_width, 0);
    EXPECT_GT(got.m_height, 0);
    EXPECT_EQ(got.m_width, expected.m_width);
    EXPECT_EQ(got.m_height, expected.m_height);
    EXPECT_EQ(got.m_mipCount, expected.m_mipCount);
    EXPECT_EQ(got.m_format, expected.m_format);
}
