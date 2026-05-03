#include "Shared/SerializationTestSupport.h"

#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Core/DScene.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Serialization/ObjectSnapshot.h"
#include "Runtime/Serialization/ObjectSnapshotReader.h"
#include "Runtime/Serialization/ObjectSnapshotWriter.h"
#include "Runtime/Test/SnapshotTestTypes.h"

#include <nlohmann/json.hpp>

#include <string>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

namespace
{
ObjectSnapshot MakeSnapshot(const std::string& className, const ObjectId& objectId, nlohmann::json fields)
{
    fields["_class"] = className;
    fields["_objectId"] = objectId.ToString();

    ObjectSnapshot snapshot;
    snapshot.rootClassName = className;
    snapshot.rootJson = { { "objects", nlohmann::json::array({ std::move(fields) }) } };
    snapshot.capturedIds = { objectId };
    return snapshot;
}
}

class ObjectSnapshotHardeningTests : public EditorSerializationTest
{
};

TEST_F(ObjectSnapshotHardeningTests, ObjectSnapshotWriter_CaptureNull_ReturnsEmptySnapshot)
{
    ObjectSnapshotWriter writer;

    const ObjectSnapshot snapshot = writer.Capture(nullptr);

    EXPECT_TRUE(snapshot.rootJson.empty());
    EXPECT_TRUE(snapshot.capturedIds.empty());
    EXPECT_TRUE(snapshot.rootClassName.empty());
}

TEST_F(ObjectSnapshotHardeningTests, ObjectSnapshotReader_RestoreEmptySnapshot_ReturnsNull)
{
    ObjectSnapshotReader reader;
    const ObjectSnapshot snapshot;

    DObject* restored = reader.Restore(snapshot, nullptr, nullptr);

    EXPECT_EQ(restored, nullptr);
}

TEST_F(ObjectSnapshotHardeningTests, ObjectSnapshotReader_RestoreUnknownClassOnly_ReturnsNull)
{
    const ObjectId objectId = ObjectId::Generate();
    ObjectSnapshot snapshot = MakeSnapshot("DMissingSnapshotClass", objectId, { { "m_value", 1.0f } });
    ObjectSnapshotReader reader;

    DObject* restored = reader.Restore(snapshot, nullptr, nullptr);

    EXPECT_EQ(restored, nullptr);
}

TEST_F(ObjectSnapshotHardeningTests, ObjectSnapshotReader_RestoreMissingObjectId_ReturnsNull)
{
    ObjectSnapshot snapshot;
    snapshot.rootClassName = "DSnapshotTestComponentA";
    snapshot.rootJson = {
        { "objects", nlohmann::json::array({
            nlohmann::json{
                { "_class", "DSnapshotTestComponentA" },
                { "m_value", 5.0f }
            }
        }) }
    };
    ObjectSnapshotReader reader;

    DObject* restored = reader.Restore(snapshot, nullptr, nullptr);

    EXPECT_EQ(restored, nullptr);
}

TEST_F(ObjectSnapshotHardeningTests, ObjectSnapshotReader_RestoreWithRegisterAsset_AddsObjectToAsset)
{
    const ObjectId objectId = ObjectId::Generate();
    ObjectSnapshot snapshot = MakeSnapshot("DSnapshotTestComponentA", objectId, { { "m_value", 12.0f } });
    auto* asset = CreateDObject<DPrimaryAsset>();
    ObjectSnapshotReader reader;

    DObject* restored = reader.Restore(snapshot, nullptr, nullptr, asset);

    ASSERT_NE(restored, nullptr);
    EXPECT_EQ(asset->FindObject(objectId), restored);
    EXPECT_EQ(restored->GetOwningAsset(), asset);
}

TEST_F(ObjectSnapshotHardeningTests, ObjectSnapshotReader_RestoreGameObject_AddsRootToWorldAndScene)
{
    DWorld sourceWorld;
    GameObject* gameObject = sourceWorld.CreateGameObject("SnapshotRoot");
    ObjectSnapshotWriter writer;
    ObjectSnapshot snapshot = writer.Capture(gameObject);
    sourceWorld.DestroyGameObject(gameObject);
    DWorld targetWorld;
    auto* scene = CreateDObject<DScene>();
    ObjectSnapshotReader reader;

    DObject* restored = reader.Restore(snapshot, &targetWorld, nullptr, nullptr, scene);

    auto* restoredGameObject = dynamic_cast<GameObject*>(restored);
    ASSERT_NE(restoredGameObject, nullptr);
    ASSERT_EQ(targetWorld.GetGameObjects().size(), 1u);
    EXPECT_EQ(targetWorld.GetGameObjects()[0], restoredGameObject);
    ASSERT_EQ(scene->GetGameObjects().size(), 1u);
    EXPECT_EQ(scene->GetGameObjects()[0], restoredGameObject);
}
