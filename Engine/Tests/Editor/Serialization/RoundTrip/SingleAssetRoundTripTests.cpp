#include "Shared/SerializationTestSupport.h"

#include "Runtime/Test/SerializationTestTypes.h"

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class SingleAssetRoundTripTests : public EditorSerializationTest
{
};

TEST_F(SingleAssetRoundTripTests, RoundTripsMixedPropertiesAndObjectReferences)
{
    const auto tempDir = MakeTempDir("DeltaRoundTripTest");

    auto* asset = CreateDObject<PA_TestAsset>();
    const AssetId assetId = AssetId::Generate();
    asset->GetHeader().m_persistentId = assetId;
    asset->GetHeader().m_className = "PA_TestAsset";

    auto* objectA = new DTestObjectA();
    const ObjectId objectAId = ObjectId::Generate();
    objectA->SetObjectId(objectAId);
    objectA->m_health = 75.5f;
    objectA->m_level = 10;
    objectA->m_isActive = true;
    objectA->m_stamina = 88.8;
    objectA->m_name = "TestHero";
    objectA->m_position = { 1.0f, 2.0f, 3.0f };
    objectA->m_rotation = { 0.0f, 0.707f, 0.0f, 0.707f };
    objectA->m_weights = { 1.0f, 2.0f, 3.0f };
    asset->AddObject(objectA);

    auto* objectB = new DTestObjectB();
    const ObjectId objectBId = ObjectId::Generate();
    objectB->SetObjectId(objectBId);
    objectB->m_label = "Companion";
    objectB->m_weight = 3.14f;
    objectB->m_targetRef = objectA;
    objectB->m_refs.push_back(objectA);
    asset->AddObject(objectB);

    const auto filePath = tempDir.Path() / "TestAsset.dasset.json";
    SaveAssetToFile(asset, filePath);

    ASSERT_TRUE(std::filesystem::exists(filePath));

    EditorAssetDatabase database;
    database.ScanAssetsFolder(tempDir.Path());
    EXPECT_NE(database.GetState(assetId), EditorAssetDatabase::AssetState::Unregistered);

    DPrimaryAsset* loaded = database.LoadAsset(assetId);
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->GetObjects().size(), 2u);

    auto* loadedObjectA = FindObjectAs<DTestObjectA>(loaded, objectAId);
    ASSERT_NE(loadedObjectA, nullptr);
    EXPECT_FLOAT_EQ(loadedObjectA->m_health, 75.5f);
    EXPECT_EQ(loadedObjectA->m_level, 10);
    EXPECT_TRUE(loadedObjectA->m_isActive);
    EXPECT_NEAR(loadedObjectA->m_stamina, 88.8, 1e-10);
    EXPECT_EQ(loadedObjectA->m_name, "TestHero");
    EXPECT_FLOAT_EQ(loadedObjectA->m_position.x, 1.0f);
    EXPECT_FLOAT_EQ(loadedObjectA->m_position.y, 2.0f);
    EXPECT_FLOAT_EQ(loadedObjectA->m_position.z, 3.0f);
    EXPECT_NEAR(loadedObjectA->m_rotation.x, 0.0f, 1e-5f);
    EXPECT_NEAR(loadedObjectA->m_rotation.y, 0.707f, 1e-3f);
    EXPECT_NEAR(loadedObjectA->m_rotation.z, 0.0f, 1e-5f);
    EXPECT_NEAR(loadedObjectA->m_rotation.w, 0.707f, 1e-3f);
    ASSERT_EQ(loadedObjectA->m_weights.size(), 3u);
    EXPECT_FLOAT_EQ(loadedObjectA->m_weights[0], 1.0f);
    EXPECT_FLOAT_EQ(loadedObjectA->m_weights[1], 2.0f);
    EXPECT_FLOAT_EQ(loadedObjectA->m_weights[2], 3.0f);

    auto* loadedObjectB = FindObjectAs<DTestObjectB>(loaded, objectBId);
    ASSERT_NE(loadedObjectB, nullptr);
    EXPECT_EQ(loadedObjectB->m_label, "Companion");
    EXPECT_NEAR(loadedObjectB->m_weight, 3.14f, 1e-5f);
    EXPECT_EQ(loadedObjectB->m_targetRef, loadedObjectA);
    ASSERT_EQ(loadedObjectB->m_refs.size(), 1u);
    EXPECT_EQ(loadedObjectB->m_refs[0], loadedObjectA);
}
