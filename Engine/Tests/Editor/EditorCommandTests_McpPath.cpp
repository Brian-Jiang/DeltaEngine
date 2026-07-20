#include "EditorCoreFixture.h"

#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/Commands/EditorCommand_SetProperty.h"
#include "Editor/Commands/PropertyValueIO.h"

#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/SceneComponent.h"

#include <memory>
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
    bool SetProperty(const AssetId& assetId, const std::string& objectId,
                     const std::string& propertyName, json valueAfter) const
    {
        EditorCommandContext ctx{ *m_core };
        return m_core->GetCommandManager().Execute(
            std::make_unique<EditorCommand_SetProperty>(
                assetId, DeltaEngine::UUID::FromString(objectId), propertyName,
                json{}, std::move(valueAfter)),
            ctx);
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
    EXPECT_FALSE(ExecCreateGameObject().empty());
}

TEST_F(McpPathTests, SetProperty_FloatProperty_RoundTrips)
{
    const AssetId sceneId = GetActiveSceneAssetId();

    const std::string goObjectId = ExecCreateGameObject();
    ASSERT_FALSE(goObjectId.empty());

    ASSERT_FALSE(ExecCreateComponent(goObjectId, "PointLight").empty());

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

    EXPECT_TRUE(SetProperty(sceneId, compId, "m_intensity", 7.5));

    DProperty* intensityProp = FindPropertyOnObject(pl, "m_intensity");
    ASSERT_NE(intensityProp, nullptr);
    EXPECT_NEAR(PropertyToJson(pl, intensityProp).get<float>(), 7.5f, 0.001f);
}

TEST_F(McpPathTests, CreateThenUndo_RemovesGameObject)
{
    ASSERT_FALSE(ExecCreateGameObject().empty());

    size_t countBefore = m_core->GetWorld()->GetGameObjects().size();
    ASSERT_GT(countBefore, 0u);

    EditorCommandContext ctx{ *m_core };
    EXPECT_TRUE(m_core->GetCommandManager().Undo(ctx));
    EXPECT_EQ(m_core->GetWorld()->GetGameObjects().size(), countBefore - 1);
}
