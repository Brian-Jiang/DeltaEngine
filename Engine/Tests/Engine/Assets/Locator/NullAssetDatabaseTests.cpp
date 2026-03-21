#include "Runtime/Assets/AssetDatabaseLocator.h"
#include "Runtime/Assets/NullAssetDatabase.h"
#include "Runtime/Core/UUID.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

TEST(NullAssetDatabaseTests, LocatorReturnsRegisteredNullDatabase)
{
    NullAssetDatabase database;
    AssetDatabaseLocator::Register(&database);

    IAssetDatabase& locatorDatabase = AssetDatabaseLocator::Get();
    const AssetId assetId = UUID::Generate();
    const ObjectId objectId = UUID::Generate();

    EXPECT_EQ(&locatorDatabase, &database);
    EXPECT_EQ(locatorDatabase.LoadAsset(assetId), nullptr);
    EXPECT_FALSE(locatorDatabase.IsLoaded(assetId));
    EXPECT_EQ(locatorDatabase.FindObject(assetId, objectId), nullptr);
    EXPECT_TRUE(locatorDatabase.FindAssetIdByPath("MissingAsset.dasset.json").IsNull());
}
