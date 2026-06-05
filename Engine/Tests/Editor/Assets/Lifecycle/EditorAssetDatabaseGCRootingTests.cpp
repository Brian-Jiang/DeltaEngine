#include "Shared/SerializationTestSupport.h"

#include "Runtime/Core/GC/GCManager.h"
#include "Runtime/Core/GC/WeakDObjectPtr.h"
#include "Runtime/Test/SerializationTestTypes.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class EditorAssetDatabaseGCRootingTests : public EditorSerializationTest
{
protected:
    void TearDown() override
    {
        GCManager& gc = GetGCManager();
        while (gc.HasPendingDestroy())
            gc.Tick();
    }
};

TEST_F(EditorAssetDatabaseGCRootingTests, LoadedAssetObjectsSurviveCollection)
{
    const auto tempDir = MakeTempDir("DeltaAssetGCRootingTest");

    EditorAssetDatabase database;

    auto* asset = CreateDObject<PA_TestAsset>();
    asset->GetHeader().m_persistentId = AssetId::Generate();
    asset->GetHeader().m_className = "PA_TestAsset";

    auto* object = CreateDObject<DTestObjectA>();
    object->SetObjectId(ObjectId::Generate());
    asset->AddObject(object);

    const auto filePath = tempDir.Path() / "RootedAsset.dasset.json";
    database.CreateAsset(filePath, asset);

    auto* orphan = CreateDObject<DTestObjectA>();

    WeakDObjectPtr<PA_TestAsset> weakAsset(asset);
    WeakDObjectPtr<DTestObjectA> weakObject(object);
    WeakDObjectPtr<DTestObjectA> weakOrphan(orphan);

    GetGCManager().CollectGarbage();

    EXPECT_TRUE(weakAsset.IsValid());
    EXPECT_TRUE(weakObject.IsValid());
    EXPECT_FALSE(weakOrphan.IsValid());
}

TEST_F(EditorAssetDatabaseGCRootingTests, RemoveObjectFromLoadedAssetAllowsCollection)
{
    const auto tempDir = MakeTempDir("DeltaAssetGCRootingUnrootTest");

    EditorAssetDatabase database;

    auto* asset = CreateDObject<PA_TestAsset>();
    asset->GetHeader().m_persistentId = AssetId::Generate();
    asset->GetHeader().m_className = "PA_TestAsset";

    auto* object = CreateDObject<DTestObjectA>();
    object->SetObjectId(ObjectId::Generate());
    asset->AddObject(object);

    const auto filePath = tempDir.Path() / "UnrootAsset.dasset.json";
    database.CreateAsset(filePath, asset);

    WeakDObjectPtr<DTestObjectA> weakObject(object);

    asset->RemoveObject(object->GetObjectId());

    GetGCManager().CollectGarbage();

    EXPECT_FALSE(weakObject.IsValid());
}
