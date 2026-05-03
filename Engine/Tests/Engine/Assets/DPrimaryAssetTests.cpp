#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Reflection/DBulkDataProperty.h"
#include "Runtime/Serialization/JsonAssetArchive.h"
#include "Runtime/Test/SerializationTestTypes.h"

#include <nlohmann/json.hpp>

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

TEST(DPrimaryAssetTests, AddObject_NullObject_AbortsProcess)
{
    auto* asset = CreateDObject<PA_TestAsset>();
    ASSERT_NE(asset, nullptr);

    EXPECT_DEATH(asset->AddObject(nullptr), "");
}

TEST(DPrimaryAssetTests, DeserializeBody_UnknownClass_SkipsEntryAndLeavesObjectsEmpty)
{
    const nlohmann::json root = {
        { "objects",
          nlohmann::json::array({ { { "_class", "TotallyUnknownAssetClassForDeserializeTest" } } }) }
    };
    JsonAssetArchive archive(root, {});

    auto* asset = CreateDObject<PA_TestAsset>();
    ASSERT_NE(asset, nullptr);

    asset->SerializeBody(archive);

    EXPECT_TRUE(asset->GetObjects().empty());
}

TEST(DPrimaryAssetTests, SerializeHeader_LoadBadMagic_PreservesMismatchMarkerInHeader)
{
    const AssetId aid = AssetId::Generate();
    const nlohmann::json root = { { "magic", "BAD!" },
                                  { "version", 1 },
                                  { "className", "PA_TestAsset" },
                                  { "assetId", aid.ToString() } };
    JsonAssetArchive archive(root, {});

    auto* asset = CreateDObject<PA_TestAsset>();
    ASSERT_NE(asset, nullptr);

    asset->SerializeHeader(archive);

    EXPECT_NE(asset->GetHeader().m_magic, 0x444C5441u);
    EXPECT_EQ(asset->GetHeader().m_fileVersion, 1u);
    EXPECT_EQ(asset->GetHeader().m_className, "PA_TestAsset");
    EXPECT_EQ(asset->GetHeader().m_persistentId, aid);
}

TEST(DPrimaryAssetTests, SerializeHeader_LoadUnsupportedVersion_PreservesDeclaredVersion)
{
    const AssetId aid = AssetId::Generate();
    const nlohmann::json root = { { "magic", "DLTA" },
                                  { "version", 999 },
                                  { "className", "PA_TestAsset" },
                                  { "assetId", aid.ToString() } };
    JsonAssetArchive archive(root, {});

    auto* asset = CreateDObject<PA_TestAsset>();
    ASSERT_NE(asset, nullptr);

    asset->SerializeHeader(archive);

    EXPECT_EQ(asset->GetHeader().m_fileVersion, 999u);
    EXPECT_EQ(asset->GetHeader().m_magic, 0x444C5441u);
}
