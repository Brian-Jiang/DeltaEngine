#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DFunction.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Test/ReflectionTestObject.h"
#include "Runtime/Test/TestComponent2.h"

#include <gtest/gtest.h>

#include <cstddef>

using namespace DeltaEngine;

TEST(ReflectionStructClassTests, PropertyChain_HierarchyNext_IncludesBaseDeclaredFields)
{
    auto& registry = GetReflectionRegistry();
    DClass* derived = registry.FindClassByName("TestComponent2");
    ASSERT_NE(derived, nullptr);

    DProperty* head = derived->GetProperties();
    ASSERT_NE(head, nullptr);

    bool sawTestInt = false;
    bool sawTestFloat = false;
    for (DProperty* p = head; p; p = p->GetHierarchyNext())
    {
        if (p->GetName() == "m_testInt")
            sawTestInt = true;
        if (p->GetName() == "m_testFloat")
            sawTestFloat = true;
    }

    EXPECT_TRUE(sawTestInt);
    EXPECT_TRUE(sawTestFloat);
}

TEST(ReflectionStructClassTests, GetFunctions_HasVoidMethod_WithZeroPayload)
{
    auto& registry = GetReflectionRegistry();
    DClass* cls = registry.FindClassByName("ReflectionTestObject");
    ASSERT_NE(cls, nullptr);

    ASSERT_FALSE(cls->GetFunctions().empty());
    ASSERT_NE(cls->FindFunctionByName("VoidMethod"), nullptr);
}

TEST(ReflectionStructClassTests, VoidMethod_Invoke_WithNullPayload_DoesNotAssert)
{
    auto& registry = GetReflectionRegistry();
    ReflectionTestObject* obj =
        registry.CreateObject<ReflectionTestObject>("ReflectionTestObject");
    ASSERT_NE(obj, nullptr);

    DFunction* fn = obj->GetClass()->FindFunctionByName("VoidMethod");
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->GetTotalSize(), 0u);

    fn->Invoke(obj, nullptr);

    registry.DestroyObject(obj);
}

TEST(ReflectionStructClassTests, ManualLifecycle_ConstructDestroyCopy_PreservesScalar)
{
    auto& registry = GetReflectionRegistry();
    DClass* cls = registry.FindClassByName("ReflectionTestObject");
    ASSERT_NE(cls, nullptr);

    constexpr size_t kSize = sizeof(ReflectionTestObject);
    constexpr size_t kAlign = alignof(ReflectionTestObject);

    alignas(kAlign) std::byte a[kSize]{};
    alignas(kAlign) std::byte b[kSize]{};

    cls->ConstructObject(static_cast<void*>(&a[0]));
    cls->ConstructObject(static_cast<void*>(&b[0]));

    auto* pa = reinterpret_cast<ReflectionTestObject*>(static_cast<void*>(&a[0]));

    DProperty* floatProp = cls->FindPropertyByName("m_rFloat");
    ASSERT_NE(floatProp, nullptr);
    const float fv = 3.75f;
    floatProp->SetValue(pa, &fv);

    cls->CopyObject(static_cast<void*>(&b[0]), static_cast<void*>(&a[0]));

    auto* pb = reinterpret_cast<ReflectionTestObject*>(static_cast<void*>(&b[0]));
    EXPECT_FLOAT_EQ(*static_cast<float*>(floatProp->GetValue(pb)), fv);

    cls->DestroyObject(static_cast<void*>(&b[0]));
    cls->DestroyObject(static_cast<void*>(&a[0]));
}

TEST(ReflectionStructClassTests, FindFunctionByName_InheritedFrom_TestComponent_Base)
{
    auto& registry = GetReflectionRegistry();
    DClass* derived = registry.FindClassByName("TestComponent2");
    ASSERT_NE(derived, nullptr);

    DFunction* addFn = derived->FindFunctionByName("TestAdd");
    ASSERT_NE(addFn, nullptr);
    EXPECT_EQ(addFn->GetDeclaringClass()->GetName(), "TestComponent");
}

TEST(ReflectionStructClassTests, OverloadedFunctions_BothRegistered)
{
    auto& registry = GetReflectionRegistry();
    DClass* cls = registry.FindClassByName("ReflectionTestObject");
    ASSERT_NE(cls, nullptr);

    EXPECT_NE(cls->FindFunctionByName("Compute"), nullptr);

    auto overloads = cls->FindOverloads("Compute");
    EXPECT_EQ(overloads.size(), 2u);
}

TEST(ReflectionStructClassTests, OverloadedFunctions_FindBySignature_Disambiguates)
{
    auto& registry = GetReflectionRegistry();
    DClass* cls = registry.FindClassByName("ReflectionTestObject");
    ASSERT_NE(cls, nullptr);

    std::string_view intParam[] = { "int32_t" };
    std::string_view floatParam[] = { "float" };

    DFunction* intFn = cls->FindFunction("Compute", std::span<const std::string_view>(intParam));
    DFunction* floatFn = cls->FindFunction("Compute", std::span<const std::string_view>(floatParam));

    ASSERT_NE(intFn, nullptr);
    ASSERT_NE(floatFn, nullptr);
    EXPECT_NE(intFn, floatFn);
    ASSERT_EQ(intFn->GetNumParams(), 1u);
    ASSERT_EQ(floatFn->GetNumParams(), 1u);
    EXPECT_EQ(intFn->GetParams()[0]->GetType(), "int32_t");
    EXPECT_EQ(floatFn->GetParams()[0]->GetType(), "float");
}

TEST(ReflectionStructClassTests, OverloadedFunctions_InvokeIntOverload_DoublesValue)
{
    auto& registry = GetReflectionRegistry();
    ReflectionTestObject* obj =
        registry.CreateObject<ReflectionTestObject>("ReflectionTestObject");
    ASSERT_NE(obj, nullptr);

    std::string_view intParam[] = { "int32_t" };
    DFunction* intFn = obj->GetClass()->FindFunction("Compute", std::span<const std::string_view>(intParam));
    ASSERT_NE(intFn, nullptr);

    std::vector<std::byte> buf(intFn->GetTotalSize());
    auto* params = reinterpret_cast<int*>(buf.data());
    params[0] = 7;

    intFn->Invoke(obj, buf.data());

    int* retSlot = reinterpret_cast<int*>(buf.data() + intFn->GetReturnValueOffset());
    EXPECT_EQ(*retSlot, 14);

    registry.DestroyObject(obj);
}
