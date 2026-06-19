#include "Runtime/Core/Delegates/DynamicDelegate.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Test/DynamicDelegateTestTypes.h"
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

float ReadTestFloat(const TestComponent* object)
{
    DProperty* prop = object->GetClass()->FindPropertyByName("m_testFloat");
    EXPECT_NE(prop, nullptr);
    return *static_cast<const float*>(prop->GetValue(object));
}

void ResetTestValues(TestComponent* object)
{
    DProperty* intProp = object->GetClass()->FindPropertyByName("m_testInt");
    DProperty* floatProp = object->GetClass()->FindPropertyByName("m_testFloat");
    ASSERT_NE(intProp, nullptr);
    ASSERT_NE(floatProp, nullptr);
    int zeroInt = 0;
    float zeroFloat = 0.0f;
    intProp->SetValue(object, &zeroInt);
    floatProp->SetValue(object, &zeroFloat);
}

}

TEST(DynamicDelegateBroadcastArgsTests, Broadcast_OneParam_InvokesReflectedHandler)
{
    auto& registry = GetReflectionRegistry();
    TestComponent* object = registry.CreateObject<TestComponent>("TestComponent");
    ASSERT_NE(object, nullptr);
    ResetTestValues(object);

    FOnIntEvent delegate;
    delegate.AddDynamic(object, "OnIntEvent");
    delegate.Broadcast(42);

    EXPECT_EQ(ReadTestInt(object), 42);

    registry.DestroyObject(object);
}

TEST(DynamicDelegateBroadcastArgsTests, Broadcast_TwoParams_InvokesReflectedHandler)
{
    auto& registry = GetReflectionRegistry();
    TestComponent* object = registry.CreateObject<TestComponent>("TestComponent");
    ASSERT_NE(object, nullptr);
    ResetTestValues(object);

    FOnTwoArgEvent delegate;
    delegate.AddDynamic(object, "OnTwoArgEvent");
    delegate.Broadcast(3, 2.5f);

    EXPECT_EQ(ReadTestInt(object), 3);
    EXPECT_FLOAT_EQ(ReadTestFloat(object), 2.5f);

    registry.DestroyObject(object);
}
