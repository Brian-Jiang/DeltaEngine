#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <string>

#include "../EditorCoreFixture.h"

#include "Editor/Commands/EditorCommandBatch.h"
#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/Commands/EditorCommand_SetProperty.h"
#include "Editor/Commands/EditorCommand_SetTestValue.h"
#include "Editor/Commands/EditorCommand_CreateGameObject.h"

#include "Runtime/Core/GameObject.h"

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class EditorCommandBatchFixture : public EditorCoreFixture
{
};

TEST_F(EditorCommandBatchFixture, EditorCommandBatch_Execute_Undo_RoundTripsSubCommands)
{
    EditorCommandContext ctx{ *m_core };
    auto& mgr = m_core->GetCommandManager();

    EditorCommandBatch batch;
    batch.Add(std::make_unique<EditorCommand_SetTestValue>("batchA", "", "x"));
    batch.Add(std::make_unique<EditorCommand_SetTestValue>("batchB", "", "y"));

    ASSERT_TRUE(mgr.Execute(std::make_unique<EditorCommandBatch>(std::move(batch)), ctx));
    const std::string vx = "x";
    const std::string vy = "y";
    EXPECT_EQ(m_core->GetTestValue("batchA"), vx);
    EXPECT_EQ(m_core->GetTestValue("batchB"), vy);

    ASSERT_TRUE(mgr.Undo(ctx));
    EXPECT_TRUE(m_core->GetTestValue("batchA").empty());
    EXPECT_TRUE(m_core->GetTestValue("batchB").empty());
}

TEST_F(EditorCommandBatchFixture, EditorCommandBatch_SerializeDeserialize_NestedBatchRoundTrips)
{
    EditorCommandBatch inner;
    inner.Add(std::make_unique<EditorCommand_SetTestValue>("nested", "", "in"));

    EditorCommandBatch outer;
    outer.Add(std::make_unique<EditorCommandBatch>(std::move(inner)));

    nlohmann::json data;
    outer.Serialize(data);

    EditorCommandBatch restored;
    restored.Deserialize(data);

    EditorCommandContext ctx{ *m_core };
    ASSERT_TRUE(restored.Execute(ctx));
    const std::string vin = "in";
    EXPECT_EQ(m_core->GetTestValue("nested"), vin);
}

TEST_F(EditorCommandBatchFixture, EditorCommandBatch_Execute_WhenSubCommandFails_ManagerDoesNotPushBatch)
{
    EditorCommandContext ctx{ *m_core };
    auto& mgr = m_core->GetCommandManager();
    const AssetId sceneId = GetActiveSceneAssetId();

    ASSERT_TRUE(mgr.Execute(std::make_unique<EditorCommand_CreateGameObject>(sceneId, "GameObject"), ctx));

    GameObject* go = nullptr;
    for (auto* g : m_core->GetWorld()->GetGameObjects())
    {
        if (g->GetName() == "New GameObject")
        {
            go = g;
            break;
        }
    }
    ASSERT_NE(go, nullptr);
    const ObjectId goId = go->GetObjectId();

    EditorCommandBatch batch;
    batch.Add(std::make_unique<EditorCommand_SetTestValue>("okKey", "", "z"));
    const ObjectId badId = ObjectId::Generate();
    batch.Add(std::make_unique<EditorCommand_SetProperty>(
        sceneId, badId, std::string("m_name"), nlohmann::json(std::string("n")), nlohmann::json(std::string("n"))));

    EXPECT_FALSE(mgr.Execute(std::make_unique<EditorCommandBatch>(std::move(batch)), ctx));
    constexpr size_t kExpectedDepth = 1u;
    EXPECT_EQ(mgr.GetUndoStackDepth(), kExpectedDepth);
    const std::string vz = "z";
    EXPECT_EQ(m_core->GetTestValue("okKey"), vz);
}

TEST_F(EditorCommandBatchFixture, EditorCommandBatch_Execute_EmptyBatch_ReturnsFalse)
{
    EditorCommandContext ctx{ *m_core };
    EditorCommandBatch batch;
    EXPECT_FALSE(batch.Execute(ctx));
}
