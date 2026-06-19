#include "Runtime/Core/Delegates/DynamicDelegate.h"
#include "Runtime/Reflection/DObjectReferenceTraversal.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Serialization/JsonAssetArchive.h"
#include "Runtime/Test/TestComponent.h"

#include <cstddef>
#include <gtest/gtest.h>

using namespace DeltaEngine;

namespace
{

struct FDelegatePropertyTestPayload
{
    FDynamicMulticastDelegate m_delegate;
};

static DDelegateProperty s_delegateProperty(
    "m_delegate",
    offsetof(FDelegatePropertyTestPayload, m_delegate));

int ReadTestInt(const TestComponent* object)
{
    DProperty* prop = object->GetClass()->FindPropertyByName("m_testInt");
    EXPECT_NE(prop, nullptr);
    return *static_cast<const int*>(prop->GetValue(object));
}

void ResetTestInt(TestComponent* object)
{
    DProperty* prop = object->GetClass()->FindPropertyByName("m_testInt");
    ASSERT_NE(prop, nullptr);
    int zero = 0;
    prop->SetValue(object, &zero);
}

}

TEST(DynamicDelegatePropertyTests, SerializeSave_WritesBindingArray)
{
    auto& registry = GetReflectionRegistry();
    TestComponent* object = registry.CreateObject<TestComponent>("TestComponent");
    ASSERT_NE(object, nullptr);

    FDelegatePropertyTestPayload payload;
    payload.m_delegate.AddDynamic(object, "TestFunction");

    JsonAssetArchive writer;
    s_delegateProperty.Serialize(writer, &payload);

    const nlohmann::json& root = writer.GetRoot();
    ASSERT_TRUE(root.contains("m_delegate"));
    ASSERT_TRUE(root["m_delegate"].is_array());
    ASSERT_EQ(root["m_delegate"].size(), 1u);
    EXPECT_EQ(root["m_delegate"][0]["functionName"], "TestFunction");
    EXPECT_EQ(root["m_delegate"][0]["object"]["objectId"], object->GetObjectId().ToString());

    registry.DestroyObject(object);
}

TEST(DynamicDelegatePropertyTests, RoundTrip_ResolveAndBroadcast)
{
    auto& registry = GetReflectionRegistry();
    TestComponent* object = registry.CreateObject<TestComponent>("TestComponent");
    ASSERT_NE(object, nullptr);
    ResetTestInt(object);

    FDelegatePropertyTestPayload payload;
    payload.m_delegate.AddDynamic(object, "TestFunction");

    JsonAssetArchive writer;
    s_delegateProperty.Serialize(writer, &payload);

    payload.m_delegate.Clear();

    JsonAssetArchive reader(writer.GetRoot(), {});
    s_delegateProperty.Serialize(reader, &payload);

    void* fieldAddr = s_delegateProperty.GetValue(&payload);
    ASSERT_NE(s_delegateProperty.GetUnresolvedBindings(fieldAddr), nullptr);

    s_delegateProperty.ResolveBindings(fieldAddr,
        [&](const ScriptPointer& sp) -> DObject*
        {
            if (sp.m_objectId == object->GetObjectId())
                return object;
            return nullptr;
        });

    payload.m_delegate.Broadcast();

    EXPECT_EQ(ReadTestInt(object), 1);

    registry.DestroyObject(object);
}

TEST(DynamicDelegatePropertyTests, VisitUnresolved_SurfacesDelegateBindings)
{
    FDelegatePropertyTestPayload payload;

    ScriptPointer sp;
    sp.m_objectId = ObjectId::Generate();
    sp.m_assetId  = AssetId::Generate();

    void* fieldAddr = s_delegateProperty.GetValue(&payload);
    s_delegateProperty.SetUnresolvedBindings(fieldAddr, { { sp, "TestFunction" } });

    int hitCount = 0;
    VisitUnresolvedDelegateBindingsInProperty(&s_delegateProperty, fieldAddr,
        [&](DDelegatePropertyBase* prop, void* addr)
        {
            if (prop == &s_delegateProperty && addr == fieldAddr)
                ++hitCount;
        });

    EXPECT_EQ(hitCount, 1);
}

TEST(DynamicDelegatePropertyTests, PropertyType_IsDelegate)
{
    EXPECT_EQ(s_delegateProperty.GetPropertyType(), EPropertyType::Delegate);
}
