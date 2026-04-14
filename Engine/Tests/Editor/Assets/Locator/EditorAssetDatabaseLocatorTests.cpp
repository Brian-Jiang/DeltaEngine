#include "Shared/SerializationTestSupport.h"

#include "Runtime/Assets/AssetDatabaseLocator.h"
#include "Runtime/Test/SerializationTestTypes.h"

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

namespace
{
struct ScopedAssetDatabaseLocatorRegistration
{
    explicit ScopedAssetDatabaseLocatorRegistration(IAssetDatabase* db) { AssetDatabaseLocator::Register(db); }
    ~ScopedAssetDatabaseLocatorRegistration() { AssetDatabaseLocator::Unregister(); }
};
}

class EditorAssetDatabaseLocatorTests : public EditorSerializationTest
{
};

TEST_F(EditorAssetDatabaseLocatorTests, LocatorReturnsRegisteredEditorDatabase)
{
    const auto tempDir = MakeTempDir("DeltaLocatorEditorDbTest");

    auto* asset = CreateDObject<PA_TestAsset>();
    const AssetId assetId = AssetId::Generate();
    asset->GetHeader().m_persistentId = assetId;
    asset->GetHeader().m_className = "PA_TestAsset";

    auto* object = new DTestObjectA();
    const ObjectId objectId = ObjectId::Generate();
    object->SetObjectId(objectId);
    object->m_name = "LocatorHero";
    asset->AddObject(object);

    const auto filePath = tempDir.Path() / "LocatorAsset.dasset.json";
    SaveAssetToFile(asset, filePath);

    EditorAssetDatabase& database = GetSharedEditorAssetDatabase();
    ScopedAssetDatabaseLocatorRegistration locatorScope(&database);
    database.ScanAssetsFolder(tempDir.Path());

    IAssetDatabase& locatorDatabase = AssetDatabaseLocator::Get();
    EXPECT_EQ(&locatorDatabase, &database);
    EXPECT_FALSE(locatorDatabase.IsLoaded(assetId));

    DPrimaryAsset* loaded = locatorDatabase.LoadAsset(assetId);
    ASSERT_NE(loaded, nullptr);
    EXPECT_TRUE(locatorDatabase.IsLoaded(assetId));

    auto* loadedObject = dynamic_cast<DTestObjectA*>(locatorDatabase.FindObject(assetId, objectId));
    ASSERT_NE(loadedObject, nullptr);
    EXPECT_EQ(loadedObject->m_name, "LocatorHero");
}
