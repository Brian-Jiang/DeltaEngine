#include "Runtime/Assets/AssetDatabaseLocator.h"
#include "Runtime/Assets/NullAssetDatabase.h"
#include "Runtime/Core/UUID.h"

#include <filesystem>

#include <gtest/gtest.h>

using namespace DeltaEngine;

namespace
{
struct ScopedAssetDatabaseLocatorRegistration
{
    explicit ScopedAssetDatabaseLocatorRegistration(IAssetDatabase* db) { AssetDatabaseLocator::Register(db); }
    ~ScopedAssetDatabaseLocatorRegistration() { AssetDatabaseLocator::Unregister(); }
};
}

TEST(NullAssetDatabaseTests, LocatorReturnsRegisteredNullDatabase)
{
    NullAssetDatabase database;
    ScopedAssetDatabaseLocatorRegistration scope(&database);

    IAssetDatabase& locatorDatabase = AssetDatabaseLocator::Get();
    const AssetId assetId = UUID::Generate();
    const ObjectId objectId = UUID::Generate();

    EXPECT_EQ(&locatorDatabase, &database);
    EXPECT_EQ(locatorDatabase.LoadAsset(assetId), nullptr);
    EXPECT_FALSE(locatorDatabase.IsLoaded(assetId));
    EXPECT_EQ(locatorDatabase.FindObject(assetId, objectId), nullptr);
    EXPECT_TRUE(locatorDatabase.FindAssetIdByPath("MissingAsset.dasset.json").IsNull());
}

TEST(NullAssetDatabaseTests, FindAssetIdByPath_UnicodePath_ReturnsNullWithoutThrowing)
{
    NullAssetDatabase database;
    ScopedAssetDatabaseLocatorRegistration scope(&database);

    IAssetDatabase& locatorDatabase = AssetDatabaseLocator::Get();
    const std::filesystem::path unicodePath = std::filesystem::path(L"path_with_spaces caf\u00e9\u4e16.dasset.json");

    EXPECT_TRUE(locatorDatabase.FindAssetIdByPath(unicodePath).IsNull());
}
