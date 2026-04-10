#include "EditorCoreFixture.h"

#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/Commands/EditorCommand_CreateGameObject.h"

#include "Runtime/Core/GameObject.h"

#include <nlohmann/json.hpp>

#include <set>
#include <string>
#include <thread>
#include <vector>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class EditorCommandTests_CommandQueue : public EditorCoreFixture
{
};

TEST_F(EditorCommandTests_CommandQueue, EditorCommand_DrainQueue_SetProperty_ExecutesOnMainThread)
{
    const AssetId sceneId = GetActiveSceneAssetId();
    nlohmann::json createData;
    createData["sceneAssetId"] = sceneId.ToString();
    createData["className"] = "GameObject";
    nlohmann::json createEnv;
    createEnv["type"] = "EditorCommand_CreateGameObject";
    createEnv["data"] = createData;
    m_core->EnqueueSerializedCommand(createEnv.dump());
    {
        std::vector<std::string> responses;
        m_core->DrainCommandQueue(responses);
    }

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
    nlohmann::json spEnv;
    spEnv["type"] = "EditorCommand_SetProperty";
    spEnv["data"] = spData;
    m_core->EnqueueSerializedCommand(spEnv.dump());
    {
        std::vector<std::string> responses;
        m_core->DrainCommandQueue(responses);
    }

    EXPECT_EQ(go->GetName(), "FromQueue");
}

TEST_F(EditorCommandTests_CommandQueue, EditorCommand_DrainQueue_UnknownType_LogsErrorAndContinues)
{
    const AssetId sceneId = GetActiveSceneAssetId();
    m_core->EnqueueSerializedCommand(R"({"type":"NonExistent","data":{}})");

    nlohmann::json createData;
    createData["sceneAssetId"] = sceneId.ToString();
    createData["className"] = "GameObject";
    nlohmann::json createEnv;
    createEnv["type"] = "EditorCommand_CreateGameObject";
    createEnv["data"] = createData;
    m_core->EnqueueSerializedCommand(createEnv.dump());

    {
        std::vector<std::string> responses;
        m_core->DrainCommandQueue(responses);
    }

    EXPECT_FALSE(m_core->GetWorld()->GetGameObjects().empty());
}

TEST_F(EditorCommandTests_CommandQueue, EditorCommand_DrainQueue_InvalidObjectId_SkipsGracefully)
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
    nlohmann::json spEnv;
    spEnv["type"] = "EditorCommand_SetProperty";
    spEnv["data"] = spData;
    m_core->EnqueueSerializedCommand(spEnv.dump());
    {
        std::vector<std::string> responses;
        m_core->DrainCommandQueue(responses);
    }

    EXPECT_EQ(m_core->GetCommandManager().GetUndoStackDepth(), depthBefore);
}

TEST_F(EditorCommandTests_CommandQueue, EditorCommand_DrainQueue_ConcurrentEnqueue_ThreadSafe)
{
    const AssetId sceneId = GetActiveSceneAssetId();
    const std::string sceneStr = sceneId.ToString();

    std::vector<std::thread> threads;
    threads.reserve(4);
    for (int t = 0; t < 4; ++t)
    {
        threads.emplace_back([this, sceneStr]()
        {
            for (int i = 0; i < 25; ++i)
            {
                nlohmann::json createData;
                createData["sceneAssetId"] = sceneStr;
                createData["className"] = "GameObject";
                nlohmann::json createEnv;
                createEnv["type"] = "EditorCommand_CreateGameObject";
                createEnv["data"] = createData;
                m_core->EnqueueSerializedCommand(createEnv.dump());
            }
        });
    }
    for (auto& th : threads)
        th.join();

    {
        std::vector<std::string> responses;
        m_core->DrainCommandQueue(responses);
    }

    std::set<ObjectId> ids;
    for (GameObject* g : m_core->GetWorld()->GetGameObjects())
        ids.insert(g->GetObjectId());

    EXPECT_EQ(ids.size(), 100u);
    EXPECT_EQ(m_core->GetWorld()->GetGameObjects().size(), 100);
}

TEST_F(EditorCommandTests_CommandQueue, EditorCommand_DrainQueue_CreateGameObject_ResponseHasObjectId)
{
    const AssetId sceneId = GetActiveSceneAssetId();
    nlohmann::json createData;
    createData["sceneAssetId"] = sceneId.ToString();
    createData["className"] = "GameObject";
    nlohmann::json createEnv;
    createEnv["type"] = "EditorCommand_CreateGameObject";
    createEnv["data"] = createData;
    m_core->EnqueueSerializedCommand(createEnv.dump());

    std::vector<std::string> responses;
    m_core->DrainCommandQueue(responses);

    ASSERT_EQ(responses.size(), 1u);
    auto resp = nlohmann::json::parse(responses[0]);
    EXPECT_TRUE(resp.value("ok", false));
    EXPECT_FALSE(resp.value("objectId", "").empty());
}
