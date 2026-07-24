#include "EditorCoreFixture.h"

#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/Commands/EditorCommand_CreateComponent.h"
#include "Editor/Commands/EditorCommand_CreateGameObject.h"
#include "Editor/Commands/EditorCommand_SetProperty.h"
#include "Editor/Commands/PropertyValueIO.h"

#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Test/SerializationTestTypes.h"

#include "SimpleMath.h"
#include <DirectXMath.h>

#include <cmath>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;
using namespace DirectX;
using namespace DirectX::SimpleMath;

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

    DProperty* posProp = FindPropertyOnObject(root, "m_localPosition");
    ASSERT_NE(posProp, nullptr);

    nlohmann::json beforeJson = PropertyToJson(root, posProp);
    ASSERT_TRUE(beforeJson.is_array() && beforeJson.size() == 3);

    nlohmann::json afterJson = beforeJson;
    afterJson[0] = beforeJson[0].get<float>() + 10.0f;

    const XMMATRIX worldBefore = root->GetWorldTransform();

    auto cmd = std::make_unique<EditorCommand_SetProperty>(
        sceneId, root->GetObjectId(), "m_localPosition", beforeJson, afterJson);
    ASSERT_TRUE(m_core->GetCommandManager().Execute(std::move(cmd), ctx));

    const XMMATRIX worldAfter = root->GetWorldTransform();
    EXPECT_FALSE(Float4x4ApproxEqual(worldBefore, worldAfter));

    ASSERT_TRUE(m_core->GetCommandManager().Undo(ctx));
    const XMMATRIX worldRestored = root->GetWorldTransform();
    EXPECT_TRUE(Float4x4ApproxEqual(worldBefore, worldRestored));
}

TEST_F(EditorCommandTests_SetProperty, EditorCommand_SetProperty_LocalEulerAngles_StaysExactAndRebuildsQuaternion)
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

    DProperty* eulerProp = FindPropertyOnObject(root, "m_localEulerAngles");
    ASSERT_NE(eulerProp, nullptr);

    const Vector3 eulerBefore = root->GetLocalRotationEulerAngles();
    const Quaternion quatBefore = root->GetLocalRotation();

    // 370 deg is intentionally out of [-180,180]; the euler hint must not be normalized.
    const nlohmann::json before = PropertyToJson(root, eulerProp);
    const nlohmann::json after = nlohmann::json::array({ 0.0f, 0.0f, 370.0f });

    auto cmd = std::make_unique<EditorCommand_SetProperty>(
        sceneId, root->GetObjectId(), "m_localEulerAngles", before, after);
    ASSERT_TRUE(m_core->GetCommandManager().Execute(std::move(cmd), ctx));

    EXPECT_FLOAT_EQ(root->GetLocalRotationEulerAngles().z, 370.0f);

    const XMVECTOR expectedQuat = XMQuaternionRotationRollPitchYaw(
        0.0f, 0.0f, XMConvertToRadians(370.0f));
    const Quaternion quatAfter = root->GetLocalRotation();
    EXPECT_NEAR(quatAfter.x, XMVectorGetX(expectedQuat), 1e-4f);
    EXPECT_NEAR(quatAfter.y, XMVectorGetY(expectedQuat), 1e-4f);
    EXPECT_NEAR(quatAfter.z, XMVectorGetZ(expectedQuat), 1e-4f);
    EXPECT_NEAR(quatAfter.w, XMVectorGetW(expectedQuat), 1e-4f);

    ASSERT_TRUE(m_core->GetCommandManager().Undo(ctx));
    const Vector3 eulerRestored = root->GetLocalRotationEulerAngles();
    EXPECT_FLOAT_EQ(eulerRestored.x, eulerBefore.x);
    EXPECT_FLOAT_EQ(eulerRestored.y, eulerBefore.y);
    EXPECT_FLOAT_EQ(eulerRestored.z, eulerBefore.z);
    const Quaternion quatRestored = root->GetLocalRotation();
    EXPECT_NEAR(quatRestored.x, quatBefore.x, 1e-4f);
    EXPECT_NEAR(quatRestored.y, quatBefore.y, 1e-4f);
    EXPECT_NEAR(quatRestored.z, quatBefore.z, 1e-4f);
    EXPECT_NEAR(quatRestored.w, quatBefore.w, 1e-4f);
}

TEST_F(EditorCommandTests_SetProperty, PropertyValueIO_ScalarVector_RoundTripsAndResizes)
{
    auto* obj = CreateDObject<DTestObjectA>();
    const ObjectId objectId = ObjectId::Generate();
    obj->SetObjectId(objectId);
    obj->m_weights = { 1.0f, 2.0f };
    m_core->GetActiveSceneAsset()->AddObject(obj);

    DProperty* weightsProp = FindPropertyOnObject(obj, "m_weights");
    ASSERT_NE(weightsProp, nullptr);

    const nlohmann::json snapshot = PropertyToJson(obj, weightsProp, *m_core);
    ASSERT_TRUE(snapshot.is_array());
    ASSERT_EQ(snapshot.size(), 2u);

    const nlohmann::json expanded = nlohmann::json::array({ 1.0f, 2.0f, 0.0f });
    ASSERT_TRUE(SetPropertyFromJson(obj, weightsProp, expanded, *m_core));
    ASSERT_EQ(obj->m_weights.size(), 3u);
    EXPECT_FLOAT_EQ(obj->m_weights[2], 0.0f);

    const nlohmann::json matrixJson = nlohmann::json::array({
        nlohmann::json::array({ 1.0f, 2.0f }),
        nlohmann::json::array({ 3.0f, 4.0f, 5.0f })
    });
    DProperty* matrixProp = FindPropertyOnObject(obj, "m_weightMatrix");
    ASSERT_NE(matrixProp, nullptr);
    ASSERT_TRUE(SetPropertyFromJson(obj, matrixProp, matrixJson, *m_core));
    ASSERT_EQ(obj->m_weightMatrix.size(), 2u);
    ASSERT_EQ(obj->m_weightMatrix[1].size(), 3u);
    EXPECT_FLOAT_EQ(obj->m_weightMatrix[1][2], 5.0f);
}

TEST_F(EditorCommandTests_SetProperty, EditorCommand_SetProperty_VectorAdd_UndoRestores)
{
    EditorCommandContext ctx{ *m_core };
    const AssetId sceneId = GetActiveSceneAssetId();

    auto* obj = CreateDObject<DTestObjectA>();
    const ObjectId objectId = ObjectId::Generate();
    obj->SetObjectId(objectId);
    obj->m_weights = { 1.0f, 2.0f };
    m_core->GetActiveSceneAsset()->AddObject(obj);

    DProperty* weightsProp = FindPropertyOnObject(obj, "m_weights");
    ASSERT_NE(weightsProp, nullptr);

    const nlohmann::json before = PropertyToJson(obj, weightsProp, *m_core);
    const nlohmann::json after = nlohmann::json::array({ 1.0f, 2.0f, 0.0f });

    auto cmd = std::make_unique<EditorCommand_SetProperty>(
        sceneId, objectId, "m_weights", before, after);
    ASSERT_TRUE(m_core->GetCommandManager().Execute(std::move(cmd), ctx));
    ASSERT_EQ(obj->m_weights.size(), 3u);

    ASSERT_TRUE(m_core->GetCommandManager().Undo(ctx));
    ASSERT_EQ(obj->m_weights.size(), 2u);
    EXPECT_FLOAT_EQ(obj->m_weights[0], 1.0f);
    EXPECT_FLOAT_EQ(obj->m_weights[1], 2.0f);
}
