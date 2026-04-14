#include "EditorCoreFixture.h"

#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/Commands/EditorCommand_CreateGameObject.h"
#include "Editor/Commands/EditorCommand_CreateComponent.h"
#include "Editor/Commands/EditorCommand_DeleteComponent.h"
#include "Editor/Commands/EditorCommand_DeleteGameObject.h"
#include "Editor/Commands/EditorCommand_ReparentSceneComponent.h"
#include "Editor/Commands/EditorCommand_RenameObject.h"

#include "Runtime/Core/DComponent.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Test/TestComponent.h"
#include "Runtime/Test/TestComponent2.h"

#include <DirectXMath.h>
#include <nlohmann/json.hpp>

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

class EditorCommandTests_SceneStructure : public EditorCoreFixture
{
};

TEST_F(EditorCommandTests_SceneStructure, EditorCommand_CreateGameObject_Execute_AddsToWorld)
{
    EditorCommandContext ctx{ *m_core };
    const AssetId sceneId = GetActiveSceneAssetId();
    auto cmd = std::make_unique<EditorCommand_CreateGameObject>(sceneId, "GameObject");
    ASSERT_TRUE(m_core->GetCommandManager().Execute(std::move(cmd), ctx));

    ObjectId created = ObjectId::Null();
    for (GameObject* g : m_core->GetWorld()->GetGameObjects())
    {
        if (g->GetName() == "New GameObject")
        {
            created = g->GetObjectId();
            break;
        }
    }
    EXPECT_FALSE(created.IsNull());
}

TEST_F(EditorCommandTests_SceneStructure, EditorCommand_CreateGameObject_Undo_RemovesFromWorld)
{
    EditorCommandContext ctx{ *m_core };
    const AssetId sceneId = GetActiveSceneAssetId();
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateGameObject>(sceneId, "GameObject"), ctx));
    ASSERT_TRUE(m_core->GetCommandManager().Undo(ctx));
    EXPECT_TRUE(m_core->GetWorld()->GetGameObjects().empty());
}

TEST_F(EditorCommandTests_SceneStructure, EditorCommand_CreateGameObject_Redo_RestoresWithSameId)
{
    EditorCommandContext ctx{ *m_core };
    const AssetId sceneId = GetActiveSceneAssetId();
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateGameObject>(sceneId, "GameObject"), ctx));

    ObjectId idAfterExecute = ObjectId::Null();
    for (GameObject* g : m_core->GetWorld()->GetGameObjects())
        idAfterExecute = g->GetObjectId();
    ASSERT_FALSE(idAfterExecute.IsNull());

    ASSERT_TRUE(m_core->GetCommandManager().Undo(ctx));
    ASSERT_TRUE(m_core->GetCommandManager().Redo(ctx));

    ObjectId idAfterRedo = ObjectId::Null();
    for (GameObject* g : m_core->GetWorld()->GetGameObjects())
        idAfterRedo = g->GetObjectId();
    EXPECT_EQ(idAfterExecute, idAfterRedo);
}

TEST_F(EditorCommandTests_SceneStructure, EditorCommand_DeleteGameObject_UndoRestoresFullSubtree)
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
    const ObjectId goId = go->GetObjectId();
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateComponent>(sceneId, goId, "Camera"), ctx));
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateComponent>(sceneId, goId, "Camera"), ctx));
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateComponent>(sceneId, goId, "Camera"), ctx));

    const std::vector<SceneComponent*>& scs = go->GetSceneComponents();
    ASSERT_EQ(scs.size(), 3u);
    SceneComponent* a = scs[1];
    SceneComponent* b = scs[2];
    const ObjectId aId = a->GetObjectId();
    const ObjectId bId = b->GetObjectId();

    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_ReparentSceneComponent>(sceneId, bId, aId), ctx));

    const XMMATRIX bWorldBefore = b->GetWorldTransform();

    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_DeleteGameObject>(sceneId, goId), ctx));

    ASSERT_TRUE(m_core->GetCommandManager().Undo(ctx));

    GameObject* goRestored = nullptr;
    for (GameObject* g : m_core->GetWorld()->GetGameObjects())
    {
        if (g->GetObjectId() == goId)
        {
            goRestored = g;
            break;
        }
    }
    ASSERT_NE(goRestored, nullptr);
    SceneComponent* bRestored = nullptr;
    SceneComponent* aRestored = nullptr;
    for (SceneComponent* sc : goRestored->GetSceneComponents())
    {
        if (sc->GetObjectId() == bId)
            bRestored = sc;
        if (sc->GetObjectId() == aId)
            aRestored = sc;
    }
    ASSERT_NE(bRestored, nullptr);
    ASSERT_NE(aRestored, nullptr);
    EXPECT_EQ(bRestored->GetParent(), aRestored);
    EXPECT_EQ(aRestored->GetParent(), goRestored->GetRootSceneComponent());
    EXPECT_TRUE(Float4x4ApproxEqual(bWorldBefore, bRestored->GetWorldTransform()));
}

TEST_F(EditorCommandTests_SceneStructure, EditorCommand_CreateComponent_Execute_AttachesToGameObject)
{
    EditorCommandContext ctx{ *m_core };
    const AssetId sceneId = GetActiveSceneAssetId();
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateGameObject>(sceneId, "GameObject"), ctx));

    GameObject* go = m_core->GetWorld()->GetGameObjects().front();
    const ObjectId goId = go->GetObjectId();

    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateComponent>(sceneId, goId, "TestComponent"), ctx));

    bool found = false;
    for (DComponent* c : go->GetComponents())
    {
        if (dynamic_cast<TestComponent*>(c))
        {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(EditorCommandTests_SceneStructure, EditorCommand_DeleteComponent_Undo_RestoresAtCorrectIndex)
{
    EditorCommandContext ctx{ *m_core };
    const AssetId sceneId = GetActiveSceneAssetId();
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateGameObject>(sceneId, "GameObject"), ctx));

    GameObject* go = m_core->GetWorld()->GetGameObjects().front();
    const ObjectId goId = go->GetObjectId();

    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateComponent>(sceneId, goId, "TestComponent"), ctx));
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateComponent>(sceneId, goId, "TestComponent2"), ctx));

    const DComponent* first = go->GetComponents()[0];
    const ObjectId firstId = first->GetObjectId();

    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_DeleteComponent>(sceneId, goId, firstId), ctx));

    ASSERT_EQ(go->GetComponents().size(), 1u);

    ASSERT_TRUE(m_core->GetCommandManager().Undo(ctx));

    ASSERT_EQ(go->GetComponents().size(), 2u);
    EXPECT_EQ(go->GetComponents()[0]->GetObjectId(), firstId);
}

TEST_F(EditorCommandTests_SceneStructure, EditorCommand_ReparentSceneComponent_MovesChildUnderParent)
{
    EditorCommandContext ctx{ *m_core };
    const AssetId sceneId = GetActiveSceneAssetId();

    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateGameObject>(sceneId, "GameObject"), ctx));
    GameObject* go = m_core->GetWorld()->GetGameObjects().back();
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateComponent>(sceneId, go->GetObjectId(), "Camera"), ctx));
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateComponent>(sceneId, go->GetObjectId(), "Camera"), ctx));
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateComponent>(sceneId, go->GetObjectId(), "Camera"), ctx));

    const std::vector<SceneComponent*>& scs = go->GetSceneComponents();
    ASSERT_EQ(scs.size(), 3u);
    SceneComponent* a = scs[1];
    SceneComponent* b = scs[2];

    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_ReparentSceneComponent>(
            sceneId, b->GetObjectId(), a->GetObjectId()),
        ctx));
    EXPECT_EQ(b->GetParent(), a);
}

TEST_F(EditorCommandTests_SceneStructure, EditorCommand_ReparentSceneComponent_RejectsDifferentGameObjects)
{
    EditorCommandContext ctx{ *m_core };
    const AssetId sceneId = GetActiveSceneAssetId();

    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateGameObject>(sceneId, "GameObject"), ctx));
    GameObject* goA = m_core->GetWorld()->GetGameObjects().back();
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateComponent>(sceneId, goA->GetObjectId(), "Camera"), ctx));
    SceneComponent* rootA = goA->GetRootSceneComponent();
    ASSERT_NE(rootA, nullptr);

    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateGameObject>(sceneId, "GameObject"), ctx));
    GameObject* goB = m_core->GetWorld()->GetGameObjects().back();
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateComponent>(sceneId, goB->GetObjectId(), "Camera"), ctx));
    SceneComponent* rootB = goB->GetRootSceneComponent();
    ASSERT_NE(rootB, nullptr);

    SceneComponent* parentBefore = rootB->GetParent();
    EXPECT_FALSE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_ReparentSceneComponent>(
            sceneId, rootB->GetObjectId(), rootA->GetObjectId()),
        ctx));
    EXPECT_EQ(rootB->GetParent(), parentBefore);
}

TEST_F(EditorCommandTests_SceneStructure, EditorCommand_RenameObject_UndoRestoresName)
{
    EditorCommandContext ctx{ *m_core };
    const AssetId sceneId = GetActiveSceneAssetId();
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateGameObject>(sceneId, "GameObject"), ctx));

    GameObject* go = m_core->GetWorld()->GetGameObjects().front();
    const ObjectId oid = go->GetObjectId();
    const std::string original = go->GetName();

    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_RenameObject>(sceneId, oid, std::string("OtherName")), ctx));
    EXPECT_EQ(go->GetName(), "OtherName");

    ASSERT_TRUE(m_core->GetCommandManager().Undo(ctx));
    EXPECT_EQ(go->GetName(), original);
}
