#include "EditorCoreFixture.h"

#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/Commands/PropertyValueIO.h"

#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/SceneComponent.h"

#include <nlohmann/json.hpp>

#include <functional>
#include <string>
#include <string_view>
#include <vector>

using json = nlohmann::json;
using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class McpPathTests : public EditorCoreFixture {};

namespace
{
SceneComponent* FindSceneComponentByClassName(GameObject* go, std::string_view className)
{
    std::function<SceneComponent*(SceneComponent*)> visit;
    visit = [&](SceneComponent* sc) -> SceneComponent*
    {
        if (!sc)
            return nullptr;
        if (sc->GetClass() && sc->GetClass()->GetName() == className)
            return sc;
        for (SceneComponent* ch : sc->GetChildren())
        {
            if (SceneComponent* f = visit(ch))
                return f;
        }
        return nullptr;
    };
    for (SceneComponent* sc : go->GetSceneComponents())
    {
        if (SceneComponent* f = visit(sc))
            return f;
    }
    return nullptr;
}
}

TEST_F(McpPathTests, CreateGameObject_ReturnsObjectId)
{
    const AssetId sceneId = GetActiveSceneAssetId();
    json data;
    data["sceneAssetId"] = sceneId.ToString();
    data["className"] = "GameObject";
    json envelope;
    envelope["type"] = "EditorCommand_CreateGameObject";
    envelope["data"] = data;

    m_core->EnqueueSerializedCommand(envelope.dump());

    std::vector<std::string> responses;
    m_core->DrainCommandQueue(responses);

    ASSERT_EQ(responses.size(), 1u);
    auto r = json::parse(responses[0]);
    EXPECT_TRUE(r["ok"].get<bool>());
    EXPECT_EQ(r["commandType"].get<std::string>(), "EditorCommand_CreateGameObject");
    EXPECT_FALSE(r["objectId"].get<std::string>().empty());
}

TEST_F(McpPathTests, SetProperty_FloatProperty_RoundTrips)
{
    const AssetId sceneId = GetActiveSceneAssetId();

    // Create a GameObject
    json createData;
    createData["sceneAssetId"] = sceneId.ToString();
    createData["className"] = "GameObject";
    json createEnv;
    createEnv["type"] = "EditorCommand_CreateGameObject";
    createEnv["data"] = createData;
    m_core->EnqueueSerializedCommand(createEnv.dump());
    std::vector<std::string> r1;
    m_core->DrainCommandQueue(r1);
    ASSERT_EQ(r1.size(), 1u);
    std::string goObjectId = json::parse(r1[0])["objectId"].get<std::string>();
    ASSERT_FALSE(goObjectId.empty());

    // Add a PointLight component
    json addCompData;
    addCompData["sceneAssetId"] = sceneId.ToString();
    addCompData["gameObjectId"] = goObjectId;
    addCompData["className"] = "PointLight";
    json addCompEnv;
    addCompEnv["type"] = "EditorCommand_CreateComponent";
    addCompEnv["data"] = addCompData;
    m_core->EnqueueSerializedCommand(addCompEnv.dump());
    std::vector<std::string> r2;
    m_core->DrainCommandQueue(r2);
    ASSERT_EQ(r2.size(), 1u);
    ASSERT_TRUE(json::parse(r2[0])["ok"].get<bool>());

    GameObject* go = nullptr;
    for (GameObject* g : m_core->GetWorld()->GetGameObjects())
    {
        if (g->GetObjectId().ToString() == goObjectId)
        {
            go = g;
            break;
        }
    }
    ASSERT_NE(go, nullptr);
    SceneComponent* pl = FindSceneComponentByClassName(go, "PointLight");
    ASSERT_NE(pl, nullptr);
    const std::string compId = pl->GetObjectId().ToString();
    ASSERT_FALSE(compId.empty());

    // Set m_intensity to 7.5
    json setPropData;
    setPropData["assetId"] = sceneId.ToString();
    setPropData["objectId"] = compId;
    setPropData["propertyName"] = "m_intensity";
    setPropData["valueAfter"] = 7.5;
    json setPropEnv;
    setPropEnv["type"] = "EditorCommand_SetProperty";
    setPropEnv["data"] = setPropData;
    m_core->EnqueueSerializedCommand(setPropEnv.dump());
    std::vector<std::string> r3;
    m_core->DrainCommandQueue(r3);
    ASSERT_EQ(r3.size(), 1u);
    EXPECT_TRUE(json::parse(r3[0])["ok"].get<bool>());

    DProperty* intensityProp = FindPropertyOnObject(pl, "m_intensity");
    ASSERT_NE(intensityProp, nullptr);
    EXPECT_NEAR(PropertyToJson(pl, intensityProp).get<float>(), 7.5f, 0.001f);
}

TEST_F(McpPathTests, UnknownCommandType_ReturnsError)
{
    m_core->EnqueueSerializedCommand(
        R"({"type":"EditorCommand_DoesNotExist","data":{}})");

    std::vector<std::string> r;
    m_core->DrainCommandQueue(r);

    ASSERT_EQ(r.size(), 1u);
    auto resp = json::parse(r[0]);
    EXPECT_FALSE(resp["ok"].get<bool>());
    EXPECT_FALSE(resp["error"].get<std::string>().empty());
}

TEST_F(McpPathTests, MalformedJson_ReturnsError)
{
    m_core->EnqueueSerializedCommand("{not valid json!!!");

    std::vector<std::string> r;
    m_core->DrainCommandQueue(r);

    ASSERT_EQ(r.size(), 1u);
    auto resp = json::parse(r[0]);
    EXPECT_FALSE(resp["ok"].get<bool>());
    EXPECT_TRUE(resp["error"].get<std::string>().find("parse error") != std::string::npos);
}

TEST_F(McpPathTests, MissingTypeField_ReturnsError)
{
    m_core->EnqueueSerializedCommand(R"({"data":{"foo":"bar"}})");

    std::vector<std::string> r;
    m_core->DrainCommandQueue(r);

    ASSERT_EQ(r.size(), 1u);
    auto resp = json::parse(r[0]);
    EXPECT_FALSE(resp["ok"].get<bool>());
    EXPECT_FALSE(resp["error"].get<std::string>().empty());
}

TEST_F(McpPathTests, CreateThenUndo_RemovesGameObject)
{
    const AssetId sceneId = GetActiveSceneAssetId();
    json data;
    data["sceneAssetId"] = sceneId.ToString();
    data["className"] = "GameObject";
    json envelope;
    envelope["type"] = "EditorCommand_CreateGameObject";
    envelope["data"] = data;
    m_core->EnqueueSerializedCommand(envelope.dump());
    std::vector<std::string> responses;
    m_core->DrainCommandQueue(responses);
    ASSERT_EQ(responses.size(), 1u);
    ASSERT_TRUE(json::parse(responses[0])["ok"].get<bool>());

    size_t countBefore = m_core->GetWorld()->GetGameObjects().size();
    ASSERT_GT(countBefore, 0u);

    EditorCommandContext ctx{ *m_core };
    EXPECT_TRUE(m_core->GetCommandManager().Undo(ctx));
    EXPECT_EQ(m_core->GetWorld()->GetGameObjects().size(), countBefore - 1);
}
