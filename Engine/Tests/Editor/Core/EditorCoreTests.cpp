#include "../EditorCoreFixture.h"

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/Mcp/McpRegistry.h"
#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Core/DWorld.h"

#include <nlohmann/json.hpp>

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

namespace
{
class EditorCoreTests : public EditorCoreFixture
{
};
}

TEST_F(EditorCoreTests, ResolveObject_NullAssetId_ReturnsNull)
{
    EXPECT_EQ(m_core->ResolveObject(AssetId::Null(), ObjectId::Generate()), nullptr);
}

TEST_F(EditorCoreTests, ResolveObject_NullObjectId_ReturnsNull)
{
    EXPECT_EQ(m_core->ResolveObject(GetActiveSceneAssetId(), ObjectId::Null()), nullptr);
}

TEST_F(EditorCoreTests, ResolveObject_UnregisteredAsset_ReturnsNull)
{
    EXPECT_EQ(m_core->ResolveObject(AssetId::Generate(), ObjectId::Generate()), nullptr);
}

TEST_F(EditorCoreTests, ResolveObject_UnknownObjectInLoadedAsset_ReturnsNull)
{
    const AssetId sceneId = GetActiveSceneAssetId();
    ASSERT_FALSE(sceneId.IsNull());
    EXPECT_EQ(m_core->ResolveObject(sceneId, ObjectId::Generate()), nullptr);
}

TEST_F(EditorCoreTests, GetIdsForObject_Null_ReturnsNullPair)
{
    const auto [aid, oid] = m_core->GetIdsForObject(nullptr);
    EXPECT_TRUE(aid.IsNull());
    EXPECT_TRUE(oid.IsNull());
}

TEST_F(EditorCoreTests, GetIdsForObject_RoundTripsLoadedSceneAsset)
{
    DPrimaryAsset* scene = m_core->GetActiveSceneAsset();
    ASSERT_NE(scene, nullptr);

    const auto& objects = scene->GetObjects();
    if (objects.empty())
        GTEST_SKIP() << "Default scene has no objects to round-trip";

    DObject* first = objects.front();
    const auto [aid, oid] = m_core->GetIdsForObject(first);
    EXPECT_EQ(aid, scene->GetAssetId());
    EXPECT_EQ(oid, first->GetObjectId());
}

TEST_F(EditorCoreTests, CreateGameObject_ReturnsObjectId)
{
    const std::string objectId = ExecCreateGameObject();
    EXPECT_FALSE(objectId.empty());
    EXPECT_EQ(m_core->GetWorld()->GetGameObjects().size(), 1u);
}

TEST_F(EditorCoreTests, SaveProjectCommand_Succeeds)
{
    const auto j = m_core->GetMcpRegistry()->DispatchCommand(
        "common", "SaveProject", *m_core, nlohmann::json::object());
    EXPECT_TRUE(j["ok"].get<bool>());
}

TEST_F(EditorCoreTests, LoadScene_NonExistentPath_DoesNothing)
{
    const auto missing = m_tempDir / "missing_scene.dasset.json";
    m_core->LoadScene(missing);

    EXPECT_NE(m_core->GetActiveSceneAsset(), nullptr);
}
