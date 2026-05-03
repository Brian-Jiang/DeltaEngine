#include "Runtime/Assets/AssetDatabaseLocator.h"
#include "Runtime/Assets/NullAssetDatabase.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

namespace
{
void RegisterSameDatabaseTwiceForDeathTest()
{
    NullAssetDatabase database;
    AssetDatabaseLocator::Register(&database);
    AssetDatabaseLocator::Register(&database);
}
}

TEST(AssetDatabaseLocatorTests, RegisterAndUnregister_RoundTrip_GetReturnsRegisteredPointer)
{
    NullAssetDatabase database;

    AssetDatabaseLocator::Register(&database);

    IAssetDatabase& locatorDatabase = AssetDatabaseLocator::Get();
    EXPECT_EQ(&locatorDatabase, &database);

    AssetDatabaseLocator::Unregister();
}

TEST(AssetDatabaseLocatorTests, Register_NullDatabase_AbortsProcess)
{
    EXPECT_DEATH(AssetDatabaseLocator::Register(nullptr), "");
}

TEST(AssetDatabaseLocatorTests, Register_Twice_AbortsProcess)
{
    EXPECT_DEATH(RegisterSameDatabaseTwiceForDeathTest(), "");
}

TEST(AssetDatabaseLocatorTests, Get_BeforeRegister_AbortsProcess)
{
    EXPECT_DEATH(static_cast<void>(AssetDatabaseLocator::Get()), "");
}

TEST(AssetDatabaseLocatorTests, Unregister_WithoutRegister_AbortsProcess)
{
    EXPECT_DEATH(AssetDatabaseLocator::Unregister(), "");
}
