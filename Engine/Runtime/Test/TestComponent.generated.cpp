#include "Runtime/Test/TestComponent.generated.h"

#include "Runtime/Test/TestComponent.h"
#include "Runtime/Core/DObject.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/DFunction.h"
#include "Runtime/Reflection/ReflectionRegistry.h"

using namespace DeltaEngine;

// ---- Thunks ----

static void TestFunction_Thunk(DObject* instance, void* params)
{
    static_cast<TestComponent*>(instance)->TestFunction();
}

static void TestAdd_Thunk(DObject* instance, void* params)
{
    int a   = *(int*)((uint8_t*)params + 0);
    int b   = *(int*)((uint8_t*)params + 4);
    int ret = static_cast<TestComponent*>(instance)->TestAdd(a, b);
    *(int*)((uint8_t*)params + 8) = ret;
}

static void TestMultiply_Thunk(DObject* instance, void* params)
{
    float x   = *(float*)((uint8_t*)params + 0);
    bool  neg = *(bool*) ((uint8_t*)params + 4);
    float ret = static_cast<TestComponent*>(instance)->TestMultiply(x, neg);
    *(float*)((uint8_t*)params + 8) = ret;
}

void Reflection::Private::ReflectionRegister_TestComponent::ReflectionRegisterFn_TestComponent()
{
    DClass* cls = new DClass("TestComponent",
                             nullptr,
                             sizeof(TestComponent),
                             alignof(TestComponent),
                             [](void* address) { new (address) TestComponent(); },
                             [](void* address) { static_cast<TestComponent*>(address)->~TestComponent(); },
                             [](void* dest, const void* src) { new (dest) TestComponent(*static_cast<const TestComponent*>(src)); },
                             nullptr
    );

    cls->AddProperty(new DFloatProperty(
        "m_testFloat",
        offsetof(TestComponent, m_testFloat)));

    cls->AddProperty(new DIntProperty(
        "m_testInt",
        offsetof(TestComponent, m_testInt)));

    cls->AddProperty(new DBoolProperty(
        "m_testBool",
        offsetof(TestComponent, m_testBool)));

    cls->AddProperty(new DDoubleProperty(
        "m_testDouble",
        offsetof(TestComponent, m_testDouble)));

    cls->AddProperty(new DStringProperty(
        "m_testString",
        offsetof(TestComponent, m_testString)));

    cls->AddProperty(new DVector3Property(
        "m_testVector3",
        offsetof(TestComponent, m_testVector3)));

    cls->AddProperty(new DQuaternionProperty(
        "m_testQuaternion",
        offsetof(TestComponent, m_testQuaternion)));

    cls->AddProperty(new DObjectPtrProperty<DTexture>(
        "m_texture",
        "DTexture",
        offsetof(TestComponent, m_texture)));

    {
        DFunction* fn = new DFunction("TestFunction", &TestFunction_Thunk, 0, 0, 0);
        cls->AddFunction(fn);
    }
    {
        DFunction* fn = new DFunction("TestAdd", &TestAdd_Thunk, 2, 12, 8);
        fn->AddParam(new DIntProperty("a", 0));
        fn->AddParam(new DIntProperty("b", 4));
        fn->SetReturnProperty(new DIntProperty("ReturnValue", 8));
        cls->AddFunction(fn);
    }
    {
        DFunction* fn = new DFunction("TestMultiply", &TestMultiply_Thunk, 2, 12, 8);
        fn->AddParam(new DFloatProperty("x", 0));
        fn->AddParam(new DBoolProperty("negate", 4));
        fn->SetReturnProperty(new DFloatProperty("ReturnValue", 8));
        cls->AddFunction(fn);
    }

    GetReflectionRegistry().RegisterDClass(cls);
}

template <>
TestComponent* DeltaEngine::CreateDObject<TestComponent>()
{
    return GetReflectionRegistry().CreateObject<TestComponent>("TestComponent");
}

static ReflectionRegistration registration_TestComponent(
    &Reflection::Private::ReflectionRegister_TestComponent::ReflectionRegisterFn_TestComponent);
