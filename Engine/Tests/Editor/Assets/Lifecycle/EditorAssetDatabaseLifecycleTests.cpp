#include "Shared/SerializationTestSupport.h"

#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Test/SerializationTestTypes.h"

#include <fstream>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class EditorAssetDatabaseLifecycleTests : public EditorSerializationTest
{
};

TEST_F(EditorAssetDatabaseLifecycleTests, DirtyFlagPropagatesThroughPropertySetValue)
{
    auto* asset = CreateDObject<PA_TestAsset>();
    asset->GetHeader().m_persistentId = AssetId::Generate();
    asset->GetHeader().m_className = "PA_TestAsset";

    auto* object = new DTestObjectA();
    object->SetObjectId(ObjectId::Generate());
    asset->AddObject(object);

    asset->ClearDirty();
    EXPECT_FALSE(asset->IsDirty());

    auto* healthProperty = dynamic_cast<DFloatProperty*>(FindProperty(object, "m_health"));
    ASSERT_NE(healthProperty, nullptr);

    float newValue = 50.0f;
    healthProperty->SetValue(object, &newValue);

    EXPECT_FLOAT_EQ(object->m_health, 50.0f);
    EXPECT_TRUE(asset->IsDirty());
}

TEST_F(EditorAssetDatabaseLifecycleTests, SaveDirtyAssetsPersistsChangesAndClearsDirty)
{
    const auto tempDir = MakeTempDir("DeltaSaveDirtyAssetsTest");

    EditorAssetDatabase database;

    auto* asset = CreateDObject<PA_TestAsset>();
    asset->GetHeader().m_persistentId = AssetId::Generate();
    asset->GetHeader().m_className = "PA_TestAsset";

    auto* object = new DTestObjectA();
    object->SetObjectId(ObjectId::Generate());
    object->m_name = "BeforeSave";
    asset->AddObject(object);

    const auto filePath = tempDir.Path() / "DirtyAsset.dasset.json";
    database.CreateAsset(filePath, asset);
    EXPECT_FALSE(asset->IsDirty());

    DProperty* nameProperty = FindProperty(object, "m_name");
    ASSERT_NE(nameProperty, nullptr);

    const std::string newName = "AfterSave";
    nameProperty->SetValue(object, &newName);
    EXPECT_TRUE(asset->IsDirty());

    database.SaveDirtyAssets();
    EXPECT_FALSE(asset->IsDirty());

    std::ifstream input(filePath);
    const auto json = nlohmann::json::parse(input);
    EXPECT_EQ(json["objects"][0]["m_name"], "AfterSave");
}

TEST_F(EditorAssetDatabaseLifecycleTests, DuplicateAssetRemapsInternalRefsAndPreservesExternalRefs)
{
    const auto tempDir = MakeTempDir("DeltaDuplicateAssetTest");

    auto* externalAsset = CreateDObject<PA_TestAsset>();
    const AssetId externalAssetId = AssetId::Generate();
    externalAsset->GetHeader().m_persistentId = externalAssetId;
    externalAsset->GetHeader().m_className = "PA_TestAsset";

    auto* externalObject = new DTestObjectA();
    const ObjectId externalObjectId = ObjectId::Generate();
    externalObject->SetObjectId(externalObjectId);
    externalObject->m_name = "ExternalTarget";
    externalAsset->AddObject(externalObject);
    SaveAssetToFile(externalAsset, tempDir.Path() / "ExternalAsset.dasset.json");

    auto* sourceAsset = CreateDObject<PA_TestAsset>();
    const AssetId sourceAssetId = AssetId::Generate();
    sourceAsset->GetHeader().m_persistentId = sourceAssetId;
    sourceAsset->GetHeader().m_className = "PA_TestAsset";

    auto* internalObject = new DTestObjectA();
    const ObjectId internalObjectId = ObjectId::Generate();
    internalObject->SetObjectId(internalObjectId);
    internalObject->m_name = "InternalTarget";
    sourceAsset->AddObject(internalObject);

    auto* referenceObject = new DTestObjectB();
    const ObjectId referenceObjectId = ObjectId::Generate();
    referenceObject->SetObjectId(referenceObjectId);
    referenceObject->m_label = "RefHolder";
    referenceObject->m_targetRef = internalObject;
    referenceObject->m_refs.push_back(internalObject);
    referenceObject->m_refs.push_back(externalObject);
    sourceAsset->AddObject(referenceObject);

    auto* meshObject = new DTestMeshData();
    meshObject->SetObjectId(ObjectId::Generate());
    meshObject->m_vertexCount = 2;
    meshObject->m_indexCount = 3;
    const uint8_t vertices[] = { 1, 2, 3, 4 };
    const uint8_t indices[] = { 9, 8, 7, 6, 5, 4 };
    meshObject->m_vertexBuffer.Set(vertices, sizeof(vertices));
    meshObject->m_indexBuffer.Set(indices, sizeof(indices));
    sourceAsset->AddObject(meshObject);

    const auto sourcePath = tempDir.Path() / "SourceAsset.dasset.json";
    SaveAssetWithBulkData(sourceAsset, sourcePath);

    EditorAssetDatabase database;
    database.ScanAssetsFolder(tempDir.Path());

    const AssetId duplicatedId = database.DuplicateAsset(sourceAssetId);
    EXPECT_FALSE(duplicatedId.IsNull());
    EXPECT_NE(duplicatedId, sourceAssetId);

    DPrimaryAsset* duplicatedAsset = database.LoadAsset(duplicatedId);
    ASSERT_NE(duplicatedAsset, nullptr);
    EXPECT_EQ(duplicatedAsset->GetObjects().size(), 3u);

    DTestObjectA* duplicatedInternal = nullptr;
    DTestObjectB* duplicatedReference = nullptr;
    DTestMeshData* duplicatedMesh = nullptr;
    for (DObject* object : duplicatedAsset->GetObjects())
    {
        if (!duplicatedInternal)
            duplicatedInternal = dynamic_cast<DTestObjectA*>(object);
        if (!duplicatedReference)
            duplicatedReference = dynamic_cast<DTestObjectB*>(object);
        if (!duplicatedMesh)
            duplicatedMesh = dynamic_cast<DTestMeshData*>(object);
    }

    ASSERT_NE(duplicatedInternal, nullptr);
    ASSERT_NE(duplicatedReference, nullptr);
    ASSERT_NE(duplicatedMesh, nullptr);
    EXPECT_NE(duplicatedInternal->GetObjectId(), internalObjectId);
    EXPECT_NE(duplicatedReference->GetObjectId(), referenceObjectId);

    ASSERT_NE(duplicatedReference->m_targetRef, nullptr);
    EXPECT_EQ(duplicatedReference->m_targetRef, duplicatedInternal);
    EXPECT_EQ(duplicatedReference->m_targetRef->GetOwningAsset()->GetAssetId(), duplicatedId);
    ASSERT_EQ(duplicatedReference->m_refs.size(), 2u);
    EXPECT_EQ(duplicatedReference->m_refs[0], duplicatedInternal);
    ASSERT_NE(duplicatedReference->m_refs[1], nullptr);
    EXPECT_EQ(duplicatedReference->m_refs[1]->GetObjectId(), externalObjectId);
    EXPECT_EQ(duplicatedReference->m_refs[1]->GetOwningAsset()->GetAssetId(), externalAssetId);

    ASSERT_EQ(duplicatedMesh->m_vertexBuffer.m_size, sizeof(vertices));
    ASSERT_EQ(duplicatedMesh->m_indexBuffer.m_size, sizeof(indices));
    for (size_t index = 0; index < sizeof(vertices); ++index)
        EXPECT_EQ(duplicatedMesh->m_vertexBuffer.m_data[index], vertices[index]);
    for (size_t index = 0; index < sizeof(indices); ++index)
        EXPECT_EQ(duplicatedMesh->m_indexBuffer.m_data[index], indices[index]);

    const auto duplicatedPath = database.GetAssetPath(duplicatedId);
    EXPECT_TRUE(std::filesystem::exists(duplicatedPath));
    EXPECT_TRUE(std::filesystem::exists(
        duplicatedPath.parent_path() / (duplicatedPath.stem().stem().string() + "_Bulk0.bin")));
    EXPECT_TRUE(std::filesystem::exists(
        duplicatedPath.parent_path() / (duplicatedPath.stem().stem().string() + "_Bulk1.bin")));
}

TEST_F(EditorAssetDatabaseLifecycleTests, DeleteAssetRemovesJsonAndBulkFiles)
{
    const auto tempDir = MakeTempDir("DeltaDeleteAssetTest");

    auto* asset = CreateDObject<PA_TestMesh>();
    const AssetId assetId = AssetId::Generate();
    asset->GetHeader().m_persistentId = assetId;
    asset->GetHeader().m_className = "PA_TestMesh";

    auto* mesh = new DTestMeshData();
    mesh->SetObjectId(ObjectId::Generate());
    const uint8_t bytes[] = { 1, 3, 5, 7 };
    mesh->m_vertexBuffer.Set(bytes, sizeof(bytes));
    mesh->m_indexBuffer.Set(bytes, sizeof(bytes));
    asset->AddObject(mesh);

    const auto filePath = tempDir.Path() / "DeleteMe.dasset.json";
    SaveAssetWithBulkData(asset, filePath);

    EditorAssetDatabase database;
    database.ScanAssetsFolder(tempDir.Path());
    EXPECT_TRUE(database.DeleteAsset(assetId));

    EXPECT_FALSE(std::filesystem::exists(filePath));
    EXPECT_FALSE(std::filesystem::exists(tempDir.Path() / "DeleteMe_Bulk0.bin"));
    EXPECT_FALSE(std::filesystem::exists(tempDir.Path() / "DeleteMe_Bulk1.bin"));
    EXPECT_TRUE(database.FindAssetIdByPath(filePath).IsNull());
    EXPECT_TRUE(database.GetAllAssets().empty());
}
