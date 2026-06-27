#include "Runtime/Core/Delegates/DynamicDelegate.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Test/TestComponent.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

namespace
{

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

TEST(DynamicDelegateTests, Broadcast_InvokesReflectedNoArgFunction)
{
    auto& registry = GetReflectionRegistry();
    TestComponent* object = registry.CreateObject<TestComponent>("TestComponent");
    ASSERT_NE(object, nullptr);
    ResetTestInt(object);

    FDynamicMulticastDelegate delegate;
    delegate.AddDynamic(object, "TestFunction");
    delegate.Broadcast();

    EXPECT_EQ(ReadTestInt(object), 1);

    registry.DestroyObject(object);
}

TEST(DynamicDelegateTests, Broadcast_SkipsDestroyedObject)
{
    auto& registry = GetReflectionRegistry();
    TestComponent* object = registry.CreateObject<TestComponent>("TestComponent");
    ASSERT_NE(object, nullptr);
    ResetTestInt(object);

    FDynamicMulticastDelegate delegate;
    delegate.AddDynamic(object, "TestFunction");

    registry.DestroyObject(object);

    delegate.Broadcast();

    EXPECT_EQ(delegate.GetBindingCount(), 1u);
}

TEST(DynamicDelegateTests, Broadcast_SkipsUnknownFunctionName)
{
    auto& registry = GetReflectionRegistry();
    TestComponent* object = registry.CreateObject<TestComponent>("TestComponent");
    ASSERT_NE(object, nullptr);
    ResetTestInt(object);

    FDynamicMulticastDelegate delegate;
    delegate.AddDynamic(object, "NotARealFunction");
    delegate.Broadcast();

    EXPECT_EQ(ReadTestInt(object), 0);

    registry.DestroyObject(object);
}
