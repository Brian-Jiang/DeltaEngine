#include "EditorCoreFixture.h"

#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/Commands/EditorCommand_CreateComponent.h"
#include "Editor/Commands/EditorCommand_CreateGameObject.h"
#include "Editor/Commands/EditorCommand_SetProperty.h"
#include "Editor/Commands/PropertyValueIO.h"

#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"

#include <DirectXMath.h>

#include <cmath>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;
using namespace DirectX;

namespace
{
bool Float4x4ApproxEqual(FXMMATRIX a, FXMMATRIX b, float eps = 1e-4f)
{
    XMFLOAT4X4 A;
    XMFLOAT4X4 B;
    XMStoreFloat4x4(&A, a);
    XMStoreFloat4x4(&B, b);
    const float* pa = &A.m[0][0];
    const float* pb = &B.m[0][0];
    for (int i = 0; i < 16; ++i)
        if (std::fabs(pa[i] - pb[i]) > eps)
            return false;
    return true;
}
}

class EditorCommandTests_SetProperty : public EditorCoreFixture
{
};

TEST_F(EditorCommandTests_SetProperty, EditorCommand_SetProperty_Execute_ChangesValue)
{
    EditorCommandContext ctx{ *m_core };
    const AssetId sceneId = GetActiveSceneAssetId();
    ASSERT_FALSE(sceneId.IsNull());

    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateGameObject>(sceneId, "GameObject"), ctx));

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

    DProperty* nameProp = FindPropertyOnObject(go, "m_name");
    ASSERT_NE(nameProp, nullptr);

    const std::string newName = "RenamedGO";
    auto cmd = std::make_unique<EditorCommand_SetProperty>(
        sceneId, go->GetObjectId(), "m_name",
        PropertyToJson(go, nameProp), nlohmann::json(newName));
    ASSERT_TRUE(m_core->GetCommandManager().Execute(std::move(cmd), ctx));
    EXPECT_EQ(go->GetName(), newName);
}

TEST_F(EditorCommandTests_SetProperty, EditorCommand_SetProperty_Undo_RevertsValue)
{
    EditorCommandContext ctx{ *m_core };
    const AssetId sceneId = GetActiveSceneAssetId();
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateGameObject>(sceneId, "GameObject"), ctx));

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
    const std::string original = go->GetName();

    DProperty* nameProp = FindPropertyOnObject(go, "m_name");
    ASSERT_NE(nameProp, nullptr);
    auto cmd = std::make_unique<EditorCommand_SetProperty>(
        sceneId, go->GetObjectId(), "m_name",
        PropertyToJson(go, nameProp), nlohmann::json(std::string("X")));
    ASSERT_TRUE(m_core->GetCommandManager().Execute(std::move(cmd), ctx));
    ASSERT_TRUE(m_core->GetCommandManager().Undo(ctx));
    EXPECT_EQ(go->GetName(), original);
}

TEST_F(EditorCommandTests_SetProperty, EditorCommand_SetProperty_Redo_ReappliesValue)
{
    EditorCommandContext ctx{ *m_core };
    const AssetId sceneId = GetActiveSceneAssetId();
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateGameObject>(sceneId, "GameObject"), ctx));

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

    DProperty* nameProp = FindPropertyOnObject(go, "m_name");
    ASSERT_NE(nameProp, nullptr);
    const std::string newName = "RedoName";
    auto cmd = std::make_unique<EditorCommand_SetProperty>(
        sceneId, go->GetObjectId(), "m_name",
        PropertyToJson(go, nameProp), nlohmann::json(newName));
    ASSERT_TRUE(m_core->GetCommandManager().Execute(std::move(cmd), ctx));
    ASSERT_TRUE(m_core->GetCommandManager().Undo(ctx));
    ASSERT_TRUE(m_core->GetCommandManager().Redo(ctx));
    EXPECT_EQ(go->GetName(), newName);
}

TEST_F(EditorCommandTests_SetProperty, EditorCommand_SetProperty_SceneComponent_TriggersTransformRecompute)
{
    EditorCommandContext ctx{ *m_core };
    const AssetId sceneId = GetActiveSceneAssetId();
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateGameObject>(sceneId, "GameObject"), ctx));

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
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateComponent>(sceneId, go->GetObjectId(), "Camera"), ctx));
    SceneComponent* root = go->GetRootSceneComponent();
    ASSERT_NE(root, nullptr);

    DProperty* tfProp = FindPropertyOnObject(root, "m_localTransform");
    ASSERT_NE(tfProp, nullptr);

    nlohmann::json beforeJson = PropertyToJson(root, tfProp);
    ASSERT_TRUE(beforeJson.is_array() && beforeJson.size() == 16);

    nlohmann::json afterJson = beforeJson;
    afterJson[12] = beforeJson[12].get<float>() + 10.0f;

    const XMMATRIX worldBefore = root->GetWorldTransform();

    auto cmd = std::make_unique<EditorCommand_SetProperty>(
        sceneId, root->GetObjectId(), "m_localTransform", beforeJson, afterJson);
    ASSERT_TRUE(m_core->GetCommandManager().Execute(std::move(cmd), ctx));

    const XMMATRIX worldAfter = root->GetWorldTransform();
    EXPECT_FALSE(Float4x4ApproxEqual(worldBefore, worldAfter));

    ASSERT_TRUE(m_core->GetCommandManager().Undo(ctx));
    const XMMATRIX worldRestored = root->GetWorldTransform();
    EXPECT_TRUE(Float4x4ApproxEqual(worldBefore, worldRestored));
}
