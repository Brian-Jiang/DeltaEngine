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

using json = nlohmann::json;
using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class McpPathTests : public EditorCoreFixture
{
protected:
    json ExecLegacy(const std::string& type, json data) const
    {
        json env;
        env["type"] = type;
        env["data"] = std::move(data);
        return m_core->ExecuteSerializedCommand(env);
    }
};

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

    const json r = ExecLegacy("EditorCommand_CreateGameObject", data);
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
    const std::string goObjectId =
        ExecLegacy("EditorCommand_CreateGameObject", createData)["objectId"].get<std::string>();
    ASSERT_FALSE(goObjectId.empty());

    // Add a PointLight component
    json addCompData;
    addCompData["sceneAssetId"] = sceneId.ToString();
    addCompData["gameObjectId"] = goObjectId;
    addCompData["className"] = "PointLight";
    ASSERT_TRUE(ExecLegacy("EditorCommand_CreateComponent", addCompData)["ok"].get<bool>());

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
    EXPECT_TRUE(ExecLegacy("EditorCommand_SetProperty", setPropData)["ok"].get<bool>());

    DProperty* intensityProp = FindPropertyOnObject(pl, "m_intensity");
    ASSERT_NE(intensityProp, nullptr);
    EXPECT_NEAR(PropertyToJson(pl, intensityProp).get<float>(), 7.5f, 0.001f);
}

TEST_F(McpPathTests, UnknownCommandType_ReturnsError)
{
    const json resp = m_core->ExecuteSerializedCommand(
        json::parse(R"({"type":"EditorCommand_DoesNotExist","data":{}})"));
    EXPECT_FALSE(resp["ok"].get<bool>());
    EXPECT_FALSE(resp["error"].get<std::string>().empty());
}

TEST_F(McpPathTests, MissingTypeField_ReturnsError)
{
    const json resp = m_core->ExecuteSerializedCommand(json::parse(R"({"data":{"foo":"bar"}})"));
    EXPECT_FALSE(resp["ok"].get<bool>());
    EXPECT_FALSE(resp["error"].get<std::string>().empty());
}

TEST_F(McpPathTests, CreateThenUndo_RemovesGameObject)
{
    const AssetId sceneId = GetActiveSceneAssetId();
    json data;
    data["sceneAssetId"] = sceneId.ToString();
    data["className"] = "GameObject";
    ASSERT_TRUE(ExecLegacy("EditorCommand_CreateGameObject", data)["ok"].get<bool>());

    size_t countBefore = m_core->GetWorld()->GetGameObjects().size();
    ASSERT_GT(countBefore, 0u);

    EditorCommandContext ctx{ *m_core };
    EXPECT_TRUE(m_core->GetCommandManager().Undo(ctx));
    EXPECT_EQ(m_core->GetWorld()->GetGameObjects().size(), countBefore - 1);
}
