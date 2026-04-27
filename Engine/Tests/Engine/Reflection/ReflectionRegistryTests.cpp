#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DFunction.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Test/TestComponent.h"
#include "Runtime/Test/TestComponent2.h"

#include <gtest/gtest.h>

#include <cstddef>

using namespace DeltaEngine;

TEST(ReflectionRegistryTests, FindsClassesAndInheritedProperties)
{
    auto& registry = GetReflectionRegistry();

    DClass* baseClass = registry.FindClassByName("TestComponent");
    DClass* derivedClass = registry.FindClassByName("TestComponent2");

    ASSERT_NE(baseClass, nullptr);
    ASSERT_NE(derivedClass, nullptr);
    EXPECT_TRUE(derivedClass->IsChildOf(baseClass));

    DProperty* ownProperty = baseClass->FindPropertyByName("m_testString");
    DProperty* inheritedProperty = derivedClass->FindPropertyByName("m_testFloat");

    ASSERT_NE(ownProperty, nullptr);
    ASSERT_NE(inheritedProperty, nullptr);
    EXPECT_EQ(ownProperty->GetType(), "std::string");
    EXPECT_EQ(inheritedProperty->GetDeclaringStruct(), baseClass);
}

TEST(ReflectionRegistryTests, CreatesAndDestroysObjectsByClassName)
{
    auto& registry = GetReflectionRegistry();

    TestComponent* object = registry.CreateObject<TestComponent>("TestComponent");

    ASSERT_NE(object, nullptr);
    EXPECT_FALSE(object->GetObjectId().IsNull());
    EXPECT_EQ(object->GetOwningAsset(), nullptr);
    EXPECT_EQ(object->GetClass(), registry.FindClassByName("TestComponent"));

    registry.DestroyObject(object);
}

TEST(ReflectionRegistryTests, EditorOnlyPropertyFlag)
{
    auto& registry = GetReflectionRegistry();

    DClass* testClass = registry.FindClassByName("TestComponent");
    ASSERT_NE(testClass, nullptr);

    DProperty* editorOnlyProp = testClass->FindPropertyByName("m_editorOnlyFloat");
    ASSERT_NE(editorOnlyProp, nullptr);
    EXPECT_TRUE(editorOnlyProp->IsEditorOnly());

    DProperty* regularProp = testClass->FindPropertyByName("m_testFloat");
    ASSERT_NE(regularProp, nullptr);
    EXPECT_FALSE(regularProp->IsEditorOnly());
}

TEST(ReflectionRegistryTests, HideInDetailsPropertyFlag)
{
    auto& registry = GetReflectionRegistry();

    DClass* testClass = registry.FindClassByName("TestComponent");
    ASSERT_NE(testClass, nullptr);

    DProperty* hiddenProp = testClass->FindPropertyByName("m_hideInDetailsFloat");
    ASSERT_NE(hiddenProp, nullptr);
    EXPECT_TRUE(hiddenProp->IsHiddenInDetails());

    DProperty* regularProp = testClass->FindPropertyByName("m_testFloat");
    ASSERT_NE(regularProp, nullptr);
    EXPECT_FALSE(regularProp->IsHiddenInDetails());
}

TEST(ReflectionRegistryTests, ExposesFunctionMetadataAndInvocation)
{
    auto& registry = GetReflectionRegistry();
    TestComponent* object = registry.CreateObject<TestComponent>("TestComponent");
    ASSERT_NE(object, nullptr);

    DClass* derivedClass = registry.FindClassByName("TestComponent2");
    ASSERT_NE(derivedClass, nullptr);

    DFunction* addFunction = derivedClass->FindFunctionByName("TestAdd");
    ASSERT_NE(addFunction, nullptr);
    EXPECT_EQ(addFunction->GetDeclaringClass(), registry.FindClassByName("TestComponent"));
    EXPECT_EQ(addFunction->GetNumParams(), 2u);
    EXPECT_EQ(addFunction->GetTotalSize(), sizeof(Reflection::Private::TestComponent_TestAdd_Params));
    EXPECT_TRUE(addFunction->HasReturnValue());
    ASSERT_NE(addFunction->GetReturnProperty(), nullptr);
    EXPECT_EQ(addFunction->GetReturnValueOffset(), offsetof(Reflection::Private::TestComponent_TestAdd_Params, returnValue));
    ASSERT_EQ(addFunction->GetParams().size(), 2u);
    EXPECT_EQ(addFunction->GetParams()[0]->GetName(), "a");
    EXPECT_EQ(addFunction->GetParams()[1]->GetName(), "b");

    Reflection::Private::TestComponent_TestAdd_Params addParams{ 3, 4, 0 };
    addFunction->Invoke(object, &addParams);
    EXPECT_EQ(addParams.returnValue, 7);

    DFunction* multiplyFunction = derivedClass->FindFunctionByName("TestMultiply");
    ASSERT_NE(multiplyFunction, nullptr);

    Reflection::Private::TestComponent_TestMultiply_Params multiplyParams{ 2.5f, true, 0.0f };
    multiplyFunction->Invoke(object, &multiplyParams);
    EXPECT_FLOAT_EQ(multiplyParams.returnValue, -2.5f);

    registry.DestroyObject(object);
}
