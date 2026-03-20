#include "Shared/SerializationTestSupport.h"

#include "Runtime/Assets/PA_CommonAssets.h"
#include "Runtime/Assets/PA_DScene.h"
#include "Runtime/Core/DMaterial.h"
#include "Runtime/Core/DMesh.h"
#include "Runtime/Core/DScene.h"
#include "Runtime/Core/DShader.h"
#include "Runtime/Core/DTexture.h"
#include "Runtime/Reflection/DClass.h"

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class TypedPrimaryAssetRoundTripTests : public EditorSerializationTest
{
};

TEST_F(TypedPrimaryAssetRoundTripTests, SceneAssetPreservesTypedPrimaryAsset)
{
    const auto tempDir = MakeTempDir("DeltaTypedSceneAssetTest");

    PA_DScene* asset = PA_DScene::Create("TypedScene");
    const AssetId assetId = asset->GetAssetId();
    DScene* scene = asset->GetScene();
    ASSERT_NE(scene, nullptr);
    const ObjectId sceneId = scene->GetObjectId();

    const auto filePath = tempDir.Path() / "TypedScene.dasset.json";
    SaveAssetToFile(asset, filePath);

    EditorAssetDatabase database;
    database.ScanAssetsFolder(tempDir.Path());
    DPrimaryAsset* loadedBase = database.LoadAsset(assetId);
    auto* loaded = dynamic_cast<PA_DScene*>(loadedBase);

    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->GetClass()->GetName(), "PA_DScene");
    EXPECT_EQ(loaded->GetHeader().m_className, "PA_DScene");
    ASSERT_NE(loaded->GetScene(), nullptr);
    EXPECT_EQ(loaded->GetScene()->GetObjectId(), sceneId);
    EXPECT_EQ(loaded->GetScene()->GetName(), "TypedScene");
}

TEST_F(TypedPrimaryAssetRoundTripTests, ShaderAssetPreservesTypedPrimaryAsset)
{
    const auto tempDir = MakeTempDir("DeltaTypedShaderAssetTest");

    DShader* shader = CreateDObject<DShader>();
    const ObjectId shaderId = shader->GetObjectId();
    PA_Shader* asset = PA_Shader::Create(shader);
    const AssetId assetId = asset->GetAssetId();

    const auto filePath = tempDir.Path() / "TypedShader.dasset.json";
    SaveAssetToFile(asset, filePath);

    EditorAssetDatabase database;
    database.ScanAssetsFolder(tempDir.Path());
    auto* loaded = dynamic_cast<PA_Shader*>(database.LoadAsset(assetId));

    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->GetHeader().m_className, "PA_Shader");
    ASSERT_NE(loaded->GetShader(), nullptr);
    EXPECT_EQ(loaded->GetShader()->GetObjectId(), shaderId);
}

TEST_F(TypedPrimaryAssetRoundTripTests, MaterialAssetPreservesTypedPrimaryAsset)
{
    const auto tempDir = MakeTempDir("DeltaTypedMaterialAssetTest");

    DMaterial* material = CreateDObject<DMaterial>();
    const ObjectId materialId = material->GetObjectId();
    PA_Material* asset = PA_Material::Create(material);
    const AssetId assetId = asset->GetAssetId();

    const auto filePath = tempDir.Path() / "TypedMaterial.dasset.json";
    SaveAssetToFile(asset, filePath);

    EditorAssetDatabase database;
    database.ScanAssetsFolder(tempDir.Path());
    auto* loaded = dynamic_cast<PA_Material*>(database.LoadAsset(assetId));

    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->GetHeader().m_className, "PA_Material");
    ASSERT_NE(loaded->GetMaterial(), nullptr);
    EXPECT_EQ(loaded->GetMaterial()->GetObjectId(), materialId);
}

TEST_F(TypedPrimaryAssetRoundTripTests, TextureAssetPreservesTypedPrimaryAsset)
{
    const auto tempDir = MakeTempDir("DeltaTypedTextureAssetTest");

    DTexture* texture = CreateDObject<DTexture>();
    const ObjectId textureId = texture->GetObjectId();
    PA_Texture* asset = PA_Texture::Create(texture);
    const AssetId assetId = asset->GetAssetId();

    const auto filePath = tempDir.Path() / "TypedTexture.dasset.json";
    SaveAssetToFile(asset, filePath);

    EditorAssetDatabase database;
    database.ScanAssetsFolder(tempDir.Path());
    auto* loaded = dynamic_cast<PA_Texture*>(database.LoadAsset(assetId));

    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->GetHeader().m_className, "PA_Texture");
    ASSERT_NE(loaded->GetTexture(), nullptr);
    EXPECT_EQ(loaded->GetTexture()->GetObjectId(), textureId);
}

TEST_F(TypedPrimaryAssetRoundTripTests, StaticMeshAssetPreservesTypedPrimaryAsset)
{
    const auto tempDir = MakeTempDir("DeltaTypedStaticMeshAssetTest");

    DMesh* mesh = CreateDObject<DMesh>();
    const ObjectId meshId = mesh->GetObjectId();
    PA_StaticMesh* asset = PA_StaticMesh::Create(mesh);
    const AssetId assetId = asset->GetAssetId();

    const auto filePath = tempDir.Path() / "TypedStaticMesh.dasset.json";
    SaveAssetToFile(asset, filePath);

    EditorAssetDatabase database;
    database.ScanAssetsFolder(tempDir.Path());
    auto* loaded = dynamic_cast<PA_StaticMesh*>(database.LoadAsset(assetId));

    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->GetHeader().m_className, "PA_StaticMesh");
    ASSERT_NE(loaded->GetStaticMesh(), nullptr);
    EXPECT_EQ(loaded->GetStaticMesh()->GetObjectId(), meshId);
}
