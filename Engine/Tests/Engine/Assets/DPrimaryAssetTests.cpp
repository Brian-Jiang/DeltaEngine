#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Reflection/DBulkDataProperty.h"
#include "Runtime/Test/SerializationTestTypes.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

TEST(DPrimaryAssetTests, AddObjectUpdatesOwnershipLookupAndDirtyState)
{
    auto* asset = CreateDObject<PA_TestAsset>();
    auto* object = CreateDObject<DTestObjectA>();

    ASSERT_NE(asset, nullptr);
    ASSERT_NE(object, nullptr);

    asset->ClearDirty();
    EXPECT_FALSE(asset->IsDirty());

    asset->AddObject(object);

    EXPECT_TRUE(asset->IsDirty());
    EXPECT_EQ(object->GetOwningAsset(), asset);
    EXPECT_EQ(asset->FindObject(object->GetObjectId()), object);
    EXPECT_EQ(asset->FindObject(ObjectId::Generate()), nullptr);
    ASSERT_EQ(asset->GetObjects().size(), 1u);
    EXPECT_EQ(asset->GetObjects().front(), object);
}

TEST(DPrimaryAssetTests, RemoveObjectClearsOwnershipAndMarksAssetDirty)
{
    auto* asset = CreateDObject<PA_TestAsset>();
    auto* object = CreateDObject<DTestObjectA>();

    ASSERT_NE(asset, nullptr);
    ASSERT_NE(object, nullptr);

    asset->AddObject(object);
    asset->ClearDirty();

    asset->RemoveObject(object->GetObjectId());

    EXPECT_TRUE(asset->IsDirty());
    EXPECT_EQ(object->GetOwningAsset(), nullptr);
    EXPECT_EQ(asset->FindObject(object->GetObjectId()), nullptr);
    EXPECT_TRUE(asset->GetObjects().empty());
}

TEST(DPrimaryAssetTests, CollectBulkPropertiesPreservesDeclarationOrderPerObject)
{
    auto* asset = CreateDObject<PA_TestMesh>();
    auto* firstObject = CreateDObject<DTestMeshData>();
    auto* secondObject = CreateDObject<DTestMeshData>();

    ASSERT_NE(asset, nullptr);
    ASSERT_NE(firstObject, nullptr);
    ASSERT_NE(secondObject, nullptr);

    asset->AddObject(firstObject);
    asset->AddObject(secondObject);

    const auto bulkProperties = asset->CollectBulkProperties();

    ASSERT_EQ(bulkProperties.size(), 4u);
    EXPECT_EQ(bulkProperties[0].second, firstObject);
    EXPECT_EQ(bulkProperties[0].first->GetName(), "m_vertexBuffer");
    EXPECT_EQ(bulkProperties[1].second, firstObject);
    EXPECT_EQ(bulkProperties[1].first->GetName(), "m_indexBuffer");
    EXPECT_EQ(bulkProperties[2].second, secondObject);
    EXPECT_EQ(bulkProperties[2].first->GetName(), "m_vertexBuffer");
    EXPECT_EQ(bulkProperties[3].second, secondObject);
    EXPECT_EQ(bulkProperties[3].first->GetName(), "m_indexBuffer");
}
