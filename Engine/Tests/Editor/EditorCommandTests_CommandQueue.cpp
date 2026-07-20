#include "EditorCoreFixture.h"

#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/Commands/EditorCommand_CreateGameObject.h"

#include "Runtime/Core/GameObject.h"

#include <nlohmann/json.hpp>

#include <set>
#include <string>
#include <vector>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class EditorCommandTests_CommandQueue : public EditorCoreFixture
{
protected:
    // Builds a legacy {type, data} envelope and executes it synchronously.
    nlohmann::json ExecLegacy(const std::string& type, nlohmann::json data) const
    {
        nlohmann::json env;
        env["type"] = type;
        env["data"] = std::move(data);
        return m_core->ExecuteSerializedCommand(env);
    }
};

TEST_F(EditorCommandTests_CommandQueue, ExecuteSerializedCommand_SetProperty_AppliesImmediately)
{
    const AssetId sceneId = GetActiveSceneAssetId();
    nlohmann::json createData;
    createData["sceneAssetId"] = sceneId.ToString();
    createData["className"] = "GameObject";
    ExecLegacy("EditorCommand_CreateGameObject", createData);

    GameObject* go = nullptr;
    for (GameObject* g : m_core->GetWorld()->GetGameObjects())
    {
        if (g->GetName() == "New GameObject")
        {
            go = g;
            break;
        }
    }
    ASSERT_NE(go, nullptr);

    nlohmann::json spData;
    spData["assetId"] = sceneId.ToString();
    spData["objectId"] = go->GetObjectId().ToString();
    spData["propertyName"] = "m_name";
    spData["valueBefore"] = nlohmann::json();
    spData["valueAfter"] = "FromQueue";
    ExecLegacy("EditorCommand_SetProperty", spData);

    EXPECT_EQ(go->GetName(), "FromQueue");
}

TEST_F(EditorCommandTests_CommandQueue, ExecuteSerializedCommand_UnknownType_ReturnsErrorAndContinues)
{
    const AssetId sceneId = GetActiveSceneAssetId();
    const nlohmann::json bad = m_core->ExecuteSerializedCommand(
        nlohmann::json::parse(R"({"type":"NonExistent","data":{}})"));
    EXPECT_FALSE(bad.value("ok", true));

    nlohmann::json createData;
    createData["sceneAssetId"] = sceneId.ToString();
    createData["className"] = "GameObject";
    ExecLegacy("EditorCommand_CreateGameObject", createData);

    EXPECT_FALSE(m_core->GetWorld()->GetGameObjects().empty());
}

TEST_F(EditorCommandTests_CommandQueue, ExecuteSerializedCommand_InvalidObjectId_SkipsGracefully)
{
    EditorCommandContext ctx{ *m_core };
    const AssetId sceneId = GetActiveSceneAssetId();
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateGameObject>(sceneId, "GameObject"), ctx));
    const size_t depthBefore = m_core->GetCommandManager().GetUndoStackDepth();

    nlohmann::json spData;
    spData["assetId"] = sceneId.ToString();
    spData["objectId"] = "11111111-1111-1111-1111-111111111111";
    spData["propertyName"] = "m_name";
    spData["valueAfter"] = "X";
    ExecLegacy("EditorCommand_SetProperty", spData);

    EXPECT_EQ(m_core->GetCommandManager().GetUndoStackDepth(), depthBefore);
}

TEST_F(EditorCommandTests_CommandQueue, ExecuteSerializedCommand_ManyCreates_ProduceDistinctObjects)
{
    const AssetId sceneId = GetActiveSceneAssetId();
    const std::string sceneStr = sceneId.ToString();

    for (int i = 0; i < 100; ++i)
    {
        nlohmann::json createData;
        createData["sceneAssetId"] = sceneStr;
        createData["className"] = "GameObject";
        ExecLegacy("EditorCommand_CreateGameObject", createData);
    }

    std::set<ObjectId> ids;
    for (GameObject* g : m_core->GetWorld()->GetGameObjects())
        ids.insert(g->GetObjectId());

    EXPECT_EQ(ids.size(), 100u);
    EXPECT_EQ(m_core->GetWorld()->GetGameObjects().size(), 100);
}

TEST_F(EditorCommandTests_CommandQueue, ExecuteSerializedCommand_CreateGameObject_ResultHasObjectId)
{
    const AssetId sceneId = GetActiveSceneAssetId();
    nlohmann::json createData;
    createData["sceneAssetId"] = sceneId.ToString();
    createData["className"] = "GameObject";
    const nlohmann::json resp = ExecLegacy("EditorCommand_CreateGameObject", createData);

    EXPECT_TRUE(resp.value("ok", false));
    EXPECT_FALSE(resp.value("objectId", "").empty());
}
