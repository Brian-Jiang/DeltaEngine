#include "Shared/SerializationTestSupport.h"

#include "Runtime/Test/SerializationTestTypes.h"

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class CrossAssetReferenceTests : public EditorSerializationTest
{
};

TEST_F(CrossAssetReferenceTests, ResolvesCrossAssetPointers)
{
    const auto tempDir = MakeTempDir("DeltaCrossAssetTest");

    const AssetId assetAId = AssetId::Generate();
    const ObjectId objectAId = ObjectId::Generate();
    const AssetId assetBId = AssetId::Generate();
    const ObjectId objectBId = ObjectId::Generate();

    const nlohmann::json assetAFile = {
        {"header", {
            {"magic", "DLTA"},
            {"version", 1},
            {"className", "PA_TestAsset"},
            {"assetId", assetAId.ToString()}
        }},
        {"objects", nlohmann::json::array({
            nlohmann::json{
                {"_class", "DTestObjectA"},
                {"_objectId", objectAId.ToString()},
                {"m_health", 80.0f},
                {"m_name", "TargetObj"}
            }
        })}
    };

    const nlohmann::json assetBFile = {
        {"header", {
            {"magic", "DLTA"},
            {"version", 1},
            {"className", "PA_TestAsset"},
            {"assetId", assetBId.ToString()}
        }},
        {"objects", nlohmann::json::array({
            nlohmann::json{
                {"_class", "DTestObjectB"},
                {"_objectId", objectBId.ToString()},
                {"m_label", "OwnerObj"},
                {"m_weight", 2.0f},
                {"m_targetRef", nlohmann::json{
                    {"assetId", assetAId.ToString()},
                    {"objectId", objectAId.ToString()}
                }}
            }
        })}
    };

    SaveJsonToFile(assetAFile, tempDir.Path() / "AssetA.dasset.json");
    SaveJsonToFile(assetBFile, tempDir.Path() / "AssetB.dasset.json");

    EditorAssetDatabase database;
    database.ScanAssetsFolder(tempDir.Path());

    DPrimaryAsset* loadedAssetB = database.LoadAsset(assetBId);
    ASSERT_NE(loadedAssetB, nullptr);
    EXPECT_TRUE(database.IsLoaded(assetAId));
    EXPECT_TRUE(database.IsLoaded(assetBId));

    auto* objectB = FindObjectAs<DTestObjectB>(loadedAssetB, objectBId);
    ASSERT_NE(objectB, nullptr);
    EXPECT_EQ(objectB->m_label, "OwnerObj");
    ASSERT_NE(objectB->m_targetRef, nullptr);
    EXPECT_EQ(objectB->m_targetRef->GetObjectId(), objectAId);

    auto* objectA = dynamic_cast<DTestObjectA*>(objectB->m_targetRef);
    ASSERT_NE(objectA, nullptr);
    EXPECT_EQ(objectA->m_name, "TargetObj");
}

TEST_F(CrossAssetReferenceTests, ResolvesCrossAssetVectorPointers)
{
    const auto tempDir = MakeTempDir("DeltaCrossAssetVectorTest");

    const AssetId assetAId = AssetId::Generate();
    const ObjectId objectAId = ObjectId::Generate();
    const AssetId assetBId = AssetId::Generate();
    const ObjectId objectBId = ObjectId::Generate();

    const nlohmann::json assetAFile = {
        {"header", {
            {"magic", "DLTA"},
            {"version", 1},
            {"className", "PA_TestAsset"},
            {"assetId", assetAId.ToString()}
        }},
        {"objects", nlohmann::json::array({
            nlohmann::json{
                {"_class", "DTestObjectA"},
                {"_objectId", objectAId.ToString()},
                {"m_health", 90.0f},
                {"m_name", "VectorTargetObj"}
            }
        })}
    };

    const nlohmann::json assetBFile = {
        {"header", {
            {"magic", "DLTA"},
            {"version", 1},
            {"className", "PA_TestAsset"},
            {"assetId", assetBId.ToString()}
        }},
        {"objects", nlohmann::json::array({
            nlohmann::json{
                {"_class", "DTestObjectB"},
                {"_objectId", objectBId.ToString()},
                {"m_label", "VectorOwnerObj"},
                {"m_weight", 4.0f},
                {"m_refs", nlohmann::json::array({
                    nlohmann::json{
                        {"assetId", assetAId.ToString()},
                        {"objectId", objectAId.ToString()}
                    }
                })}
            }
        })}
    };

    SaveJsonToFile(assetAFile, tempDir.Path() / "VectorAssetA.dasset.json");
    SaveJsonToFile(assetBFile, tempDir.Path() / "VectorAssetB.dasset.json");

    EditorAssetDatabase database;
    database.ScanAssetsFolder(tempDir.Path());

    DPrimaryAsset* loadedAssetB = database.LoadAsset(assetBId);
    ASSERT_NE(loadedAssetB, nullptr);
    EXPECT_TRUE(database.IsLoaded(assetAId));
    EXPECT_TRUE(database.IsLoaded(assetBId));

    auto* objectB = FindObjectAs<DTestObjectB>(loadedAssetB, objectBId);
    ASSERT_NE(objectB, nullptr);
    EXPECT_EQ(objectB->m_label, "VectorOwnerObj");
    EXPECT_EQ(objectB->m_targetRef, nullptr);
    ASSERT_EQ(objectB->m_refs.size(), 1u);
    ASSERT_NE(objectB->m_refs[0], nullptr);
    EXPECT_EQ(objectB->m_refs[0]->GetObjectId(), objectAId);

    auto* objectA = dynamic_cast<DTestObjectA*>(objectB->m_refs[0]);
    ASSERT_NE(objectA, nullptr);
    EXPECT_EQ(objectA->m_name, "VectorTargetObj");
}

TEST_F(CrossAssetReferenceTests, LoadsCyclicDependenciesWithoutInfiniteRecursion)
{
    const auto tempDir = MakeTempDir("DeltaCyclicTest");

    const AssetId assetAId = AssetId::Generate();
    const ObjectId objectAId = ObjectId::Generate();
    const AssetId assetBId = AssetId::Generate();
    const ObjectId objectBId = ObjectId::Generate();

    const nlohmann::json assetAFile = {
        {"header", {
            {"magic", "DLTA"},
            {"version", 1},
            {"className", "PA_TestAsset"},
            {"assetId", assetAId.ToString()}
        }},
        {"objects", nlohmann::json::array({
            nlohmann::json{
                {"_class", "DTestObjectB"},
                {"_objectId", objectAId.ToString()},
                {"m_label", "InAssetA"},
                {"m_targetRef", nlohmann::json{
                    {"assetId", assetBId.ToString()},
                    {"objectId", objectBId.ToString()}
                }}
            }
        })}
    };

    const nlohmann::json assetBFile = {
        {"header", {
            {"magic", "DLTA"},
            {"version", 1},
            {"className", "PA_TestAsset"},
            {"assetId", assetBId.ToString()}
        }},
        {"objects", nlohmann::json::array({
            nlohmann::json{
                {"_class", "DTestObjectB"},
                {"_objectId", objectBId.ToString()},
                {"m_label", "InAssetB"},
                {"m_targetRef", nlohmann::json{
                    {"assetId", assetAId.ToString()},
                    {"objectId", objectAId.ToString()}
                }}
            }
        })}
    };

    SaveJsonToFile(assetAFile, tempDir.Path() / "CycleA.dasset.json");
    SaveJsonToFile(assetBFile, tempDir.Path() / "CycleB.dasset.json");

    EditorAssetDatabase database;
    database.ScanAssetsFolder(tempDir.Path());

    DPrimaryAsset* loadedAssetA = database.LoadAsset(assetAId);
    ASSERT_NE(loadedAssetA, nullptr);
    EXPECT_TRUE(database.IsLoaded(assetAId));
    EXPECT_TRUE(database.IsLoaded(assetBId));

    auto* objectA = FindObjectAs<DTestObjectB>(loadedAssetA, objectAId);
    ASSERT_NE(objectA, nullptr);
    ASSERT_NE(objectA->m_targetRef, nullptr);
    EXPECT_EQ(objectA->m_targetRef->GetObjectId(), objectBId);

    DPrimaryAsset* loadedAssetB = database.LoadAsset(assetBId);
    ASSERT_NE(loadedAssetB, nullptr);
    auto* objectB = FindObjectAs<DTestObjectB>(loadedAssetB, objectBId);
    ASSERT_NE(objectB, nullptr);
    ASSERT_NE(objectB->m_targetRef, nullptr);
    EXPECT_EQ(objectB->m_targetRef->GetObjectId(), objectAId);
}

TEST_F(CrossAssetReferenceTests, BrokenReferenceResolvesToNullptr)
{
    const auto tempDir = MakeTempDir("DeltaBrokenRefTest");

    const AssetId assetId = AssetId::Generate();
    const ObjectId objectId = ObjectId::Generate();

    const nlohmann::json file = {
        {"header", {
            {"magic", "DLTA"},
            {"version", 1},
            {"className", "PA_TestAsset"},
            {"assetId", assetId.ToString()}
        }},
        {"objects", nlohmann::json::array({
            nlohmann::json{
                {"_class", "DTestObjectB"},
                {"_objectId", objectId.ToString()},
                {"m_label", "HasBrokenRef"},
                {"m_targetRef", nlohmann::json{
                    {"assetId", AssetId::Generate().ToString()},
                    {"objectId", ObjectId::Generate().ToString()}
                }}
            }
        })}
    };

    SaveJsonToFile(file, tempDir.Path() / "BrokenRef.dasset.json");

    EditorAssetDatabase database;
    database.ScanAssetsFolder(tempDir.Path());
    DPrimaryAsset* loaded = database.LoadAsset(assetId);

    ASSERT_NE(loaded, nullptr);
    auto* object = FindObjectAs<DTestObjectB>(loaded, objectId);
    ASSERT_NE(object, nullptr);
    EXPECT_EQ(object->m_targetRef, nullptr);
}
