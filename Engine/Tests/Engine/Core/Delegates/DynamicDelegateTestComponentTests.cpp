#include "Runtime/Core/Delegates/DynamicDelegate.h"
#include "Runtime/Reflection/DObjectReferenceTraversal.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Serialization/JsonAssetArchive.h"
#include "Runtime/Test/TestComponent.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

namespace
{

FTestComponentEvent* GetOnTestEvent(TestComponent* obj)
{
    DProperty* prop = obj->GetClass()->FindPropertyByName("OnTestEvent");
    EXPECT_NE(prop, nullptr);
    EXPECT_EQ(prop->GetPropertyType(), EPropertyType::Delegate);
    return static_cast<FTestComponentEvent*>(prop->GetValue(obj));
}

int ReadTestInt(const TestComponent* object)
{
    DProperty* prop = object->GetClass()->FindPropertyByName("m_testInt");
    EXPECT_NE(prop, nullptr);
    return *static_cast<const int*>(prop->GetValue(object));
}

}

TEST(DynamicDelegateTestComponentTests, BindAndBroadcast_CrossInstance)
{
    auto& registry = GetReflectionRegistry();
    TestComponent* listener = registry.CreateObject<TestComponent>("TestComponent");
    TestComponent* emitter = registry.CreateObject<TestComponent>("TestComponent");
    ASSERT_NE(listener, nullptr);
    ASSERT_NE(emitter, nullptr);
    TestComponent::ResetEventReceiveTracking();

    FTestComponentEvent* onTestEvent = GetOnTestEvent(emitter);
    ASSERT_NE(onTestEvent, nullptr);
    onTestEvent->AddDynamic(listener, "OnTestEventReceived");

    emitter->BroadcastTestEvent(42);

    EXPECT_EQ(TestComponent::GetLastReceivedValue(), 42);
    EXPECT_EQ(TestComponent::GetReceiveCount(), 1);

    registry.DestroyObject(listener);
    registry.DestroyObject(emitter);
}

TEST(DynamicDelegateTestComponentTests, MultiSubscriber_BothHandlersFire)
{
    auto& registry = GetReflectionRegistry();
    TestComponent* listenerA = registry.CreateObject<TestComponent>("TestComponent");
    TestComponent* listenerB = registry.CreateObject<TestComponent>("TestComponent");
    TestComponent* emitter = registry.CreateObject<TestComponent>("TestComponent");
    ASSERT_NE(listenerA, nullptr);
    ASSERT_NE(listenerB, nullptr);
    ASSERT_NE(emitter, nullptr);
    TestComponent::ResetEventReceiveTracking();

    FTestComponentEvent* onTestEvent = GetOnTestEvent(emitter);
    ASSERT_NE(onTestEvent, nullptr);
    onTestEvent->AddDynamic(listenerA, "OnTestEventReceived");
    onTestEvent->AddDynamic(listenerB, "OnTestEventReceived");

    emitter->BroadcastTestEvent(7);

    EXPECT_EQ(TestComponent::GetLastReceivedValue(), 7);
    EXPECT_EQ(TestComponent::GetReceiveCount(), 2);

    registry.DestroyObject(listenerA);
    registry.DestroyObject(listenerB);
    registry.DestroyObject(emitter);
}

TEST(DynamicDelegateTestComponentTests, SerializeRoundTrip_ResolveAndBroadcast)
{
    auto& registry = GetReflectionRegistry();
    TestComponent* listener = registry.CreateObject<TestComponent>("TestComponent");
    TestComponent* emitter = registry.CreateObject<TestComponent>("TestComponent");
    ASSERT_NE(listener, nullptr);
    ASSERT_NE(emitter, nullptr);
    TestComponent::ResetEventReceiveTracking();

    FTestComponentEvent* onTestEvent = GetOnTestEvent(emitter);
    ASSERT_NE(onTestEvent, nullptr);
    onTestEvent->AddDynamic(listener, "OnTestEventReceived");

    DProperty* delegateProp = emitter->GetClass()->FindPropertyByName("OnTestEvent");
    ASSERT_NE(delegateProp, nullptr);
    auto* delegatePropBase = dynamic_cast<DDelegatePropertyBase*>(delegateProp);
    ASSERT_NE(delegatePropBase, nullptr);

    JsonAssetArchive writer;
    delegateProp->Serialize(writer, emitter);

    onTestEvent->Clear();

    JsonAssetArchive reader(writer.GetRoot(), {});
    delegateProp->Serialize(reader, emitter);

    void* fieldAddr = delegateProp->GetValue(emitter);
    ASSERT_NE(delegatePropBase->GetUnresolvedBindings(fieldAddr), nullptr);

    ResolveUnresolvedDelegateBindingsInStruct(emitter->GetClass(), emitter,
        [&](const ScriptPointer& sp) -> DObject*
        {
            if (sp.m_objectId == listener->GetObjectId())
                return listener;
            return nullptr;
        });

    emitter->BroadcastTestEvent(99);

    EXPECT_EQ(TestComponent::GetLastReceivedValue(), 99);
    EXPECT_EQ(TestComponent::GetReceiveCount(), 1);

    registry.DestroyObject(listener);
    registry.DestroyObject(emitter);
}

TEST(DynamicDelegateTestComponentTests, StaleListener_GC_NoCrash)
{
    auto& registry = GetReflectionRegistry();
    TestComponent* listener = registry.CreateObject<TestComponent>("TestComponent");
    TestComponent* emitter = registry.CreateObject<TestComponent>("TestComponent");
    ASSERT_NE(listener, nullptr);
    ASSERT_NE(emitter, nullptr);
    TestComponent::ResetEventReceiveTracking();

    FTestComponentEvent* onTestEvent = GetOnTestEvent(emitter);
    ASSERT_NE(onTestEvent, nullptr);
    onTestEvent->AddDynamic(listener, "OnTestEventReceived");

    registry.DestroyObject(listener);

    emitter->BroadcastTestEvent(1);

    EXPECT_EQ(TestComponent::GetReceiveCount(), 0);
    EXPECT_EQ(onTestEvent->GetBindingCount(), 1u);

    registry.DestroyObject(emitter);
}
