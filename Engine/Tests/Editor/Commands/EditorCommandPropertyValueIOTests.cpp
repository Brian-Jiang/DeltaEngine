#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "../EditorCoreFixture.h"

#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/Commands/EditorCommand_CreateComponent.h"
#include "Editor/Commands/EditorCommand_CreateGameObject.h"
#include "Editor/Commands/PropertyValueIO.h"

#include "Runtime/Core/DComponent.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Test/ReflectionTestObject.h"
#include "Runtime/Test/TestComponent.h"

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class EditorCommandPropertyValueIOFixture : public EditorCoreFixture
{
protected:
    static TestComponent* FindTestComponent(GameObject* go)
    {
        if (!go)
            return nullptr;
        for (DComponent* c : go->GetComponents())
            if (auto* tc = dynamic_cast<TestComponent*>(c))
                return tc;
        return nullptr;
    }

    static ReflectionTestObject* FindReflectionTestObject(GameObject* go)
    {
        if (!go)
            return nullptr;
        for (DComponent* c : go->GetComponents())
            if (auto* rto = dynamic_cast<ReflectionTestObject*>(c))
                return rto;
        return nullptr;
    }
};

TEST_F(EditorCommandPropertyValueIOFixture, EditorCommand_PropertyValueIO_FloatRoundTrip_ReturnsEquivalentJson)
{
    EditorCommandContext ctx{ *m_core };
    const AssetId sceneId = GetActiveSceneAssetId();
    auto& mgr = m_core->GetCommandManager();

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

    ASSERT_TRUE(mgr.Execute(
        std::make_unique<EditorCommand_CreateComponent>(sceneId, go->GetObjectId(), "TestComponent"), ctx));

    TestComponent* tc = FindTestComponent(go);
    ASSERT_NE(tc, nullptr);

    DProperty* fp = FindPropertyOnObject(tc, "m_testFloat");
    ASSERT_NE(fp, nullptr);

    ASSERT_TRUE(SetPropertyFromJson(tc, fp, nlohmann::json(42.25f)));
    const nlohmann::json before = PropertyToJson(tc, fp);
    ASSERT_TRUE(before.is_number());
    EXPECT_FLOAT_EQ(before.get<float>(), 42.25f);

    ASSERT_TRUE(SetPropertyFromJson(tc, fp, nlohmann::json(0.0f)));
    ASSERT_TRUE(SetPropertyFromJson(tc, fp, before));
    ASSERT_TRUE(PropertyToJson(tc, fp).is_number());
    EXPECT_FLOAT_EQ(PropertyToJson(tc, fp).get<float>(), 42.25f);
}

TEST_F(EditorCommandPropertyValueIOFixture,
       EditorCommand_PropertyValueIO_SetFloat_FromNonNumericJson_ReturnsFalseWithoutEscape)
{
    EditorCommandContext ctx{ *m_core };
    const AssetId sceneId = GetActiveSceneAssetId();
    auto& mgr = m_core->GetCommandManager();

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

    ASSERT_TRUE(mgr.Execute(
        std::make_unique<EditorCommand_CreateComponent>(sceneId, go->GetObjectId(), "TestComponent"), ctx));

    TestComponent* tc = FindTestComponent(go);
    ASSERT_NE(tc, nullptr);

    DProperty* fp = FindPropertyOnObject(tc, "m_testFloat");
    ASSERT_NE(fp, nullptr);

    ASSERT_TRUE(SetPropertyFromJson(tc, fp, nlohmann::json(7.0f)));
    ASSERT_TRUE(PropertyToJson(tc, fp).is_number());

    EXPECT_FALSE(SetPropertyFromJson(tc, fp, nlohmann::json("not-a-number")));
    EXPECT_FLOAT_EQ(PropertyToJson(tc, fp).get<float>(), 7.0f);
}

TEST_F(EditorCommandPropertyValueIOFixture, EditorCommand_PropertyValueIO_EnumRoundTrip_ReturnsEquivalentJson)
{
    EditorCommandContext ctx{ *m_core };
    const AssetId sceneId = GetActiveSceneAssetId();
    auto& mgr = m_core->GetCommandManager();

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

    ASSERT_TRUE(mgr.Execute(
        std::make_unique<EditorCommand_CreateComponent>(sceneId, go->GetObjectId(), "ReflectionTestObject"), ctx));

    ReflectionTestObject* rto = FindReflectionTestObject(go);
    ASSERT_NE(rto, nullptr);

    DProperty* ep = FindPropertyOnObject(rto, "m_rEnum");
    ASSERT_NE(ep, nullptr);

    ASSERT_TRUE(SetPropertyFromJson(rto, ep, nlohmann::json(1)));
    const nlohmann::json before = PropertyToJson(rto, ep);
    ASSERT_TRUE(before.is_number_integer());
    EXPECT_EQ(before.get<int>(), 1);

    ASSERT_TRUE(SetPropertyFromJson(rto, ep, nlohmann::json(0)));
    ASSERT_TRUE(SetPropertyFromJson(rto, ep, before));
    ASSERT_TRUE(PropertyToJson(rto, ep).is_number_integer());
    EXPECT_EQ(PropertyToJson(rto, ep).get<int>(), 1);
}

TEST_F(EditorCommandPropertyValueIOFixture,
       EditorCommand_PropertyValueIO_SetEnum_FromStringJson_ReturnsFalseWithoutEscape)
{
    EditorCommandContext ctx{ *m_core };
    const AssetId sceneId = GetActiveSceneAssetId();
    auto& mgr = m_core->GetCommandManager();

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

    ASSERT_TRUE(mgr.Execute(
        std::make_unique<EditorCommand_CreateComponent>(sceneId, go->GetObjectId(), "ReflectionTestObject"), ctx));

    ReflectionTestObject* rto = FindReflectionTestObject(go);
    ASSERT_NE(rto, nullptr);

    DProperty* ep = FindPropertyOnObject(rto, "m_rEnum");
    ASSERT_NE(ep, nullptr);

    ASSERT_TRUE(SetPropertyFromJson(rto, ep, nlohmann::json(0)));
    ASSERT_TRUE(PropertyToJson(rto, ep).is_number_integer());

    EXPECT_FALSE(SetPropertyFromJson(rto, ep, nlohmann::json("Bar")));
    EXPECT_EQ(PropertyToJson(rto, ep).get<int>(), 0);
}

TEST_F(EditorCommandPropertyValueIOFixture, EditorCommand_PropertyValueIO_NullProperty_ReturnsNullJson)
{
    EditorCommandContext ctx{ *m_core };
    auto& mgr = m_core->GetCommandManager();

    ASSERT_TRUE(mgr.Execute(std::make_unique<EditorCommand_CreateGameObject>(GetActiveSceneAssetId(), "GameObject"), ctx));
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

    const nlohmann::json out = PropertyToJson(go, nullptr);
    EXPECT_TRUE(out.is_null());
}
