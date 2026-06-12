#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Core/GC/GCManager.h"
#include "Runtime/Core/GC/StrongDObjectPtr.h"
#include "Runtime/Core/GC/WeakDObjectPtr.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Test/SerializationTestTypes.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

namespace
{
class DPrimaryAssetGCRootingTests : public ::testing::Test
{
protected:
    void TearDown() override
    {
        GetGCManager().DrainPendingDestroyWithTimeout();
    }
};
}

TEST_F(DPrimaryAssetGCRootingTests, AddObjectRootsSurviveCollection)
{
    auto* asset = CreateDObject<PA_TestAsset>();
    StrongDObjectPtr<PA_TestAsset> assetRoot(asset);

    auto* object = CreateDObject<DTestObjectA>();
    asset->AddObject(object);

    auto* orphan = CreateDObject<DTestObjectA>();

    WeakDObjectPtr<PA_TestAsset> weakAsset(asset);
    WeakDObjectPtr<DTestObjectA> weakObject(object);
    WeakDObjectPtr<DTestObjectA> weakOrphan(orphan);

    GetGCManager().CollectGarbage();

    EXPECT_TRUE(weakAsset.IsValid());
    EXPECT_TRUE(weakObject.IsValid());
    EXPECT_FALSE(weakOrphan.IsValid());

    asset->ClearObjectRoots();
    assetRoot.Reset();
    GetGCManager().CollectGarbage();
}

TEST_F(DPrimaryAssetGCRootingTests, RemoveObjectUnrootsForCollection)
{
    auto* asset = CreateDObject<PA_TestAsset>();
    StrongDObjectPtr<PA_TestAsset> assetRoot(asset);

    auto* object = CreateDObject<DTestObjectA>();
    const ObjectId objectId = object->GetObjectId();
    asset->AddObject(object);

    WeakDObjectPtr<DTestObjectA> weakObject(object);
    WeakDObjectPtr<PA_TestAsset> weakAsset(asset);

    asset->RemoveObject(objectId);

    GetGCManager().CollectGarbage();

    EXPECT_FALSE(weakObject.IsValid());
    EXPECT_TRUE(weakAsset.IsValid());

    assetRoot.Reset();
    GetGCManager().CollectGarbage();
    EXPECT_FALSE(weakAsset.IsValid());
}
