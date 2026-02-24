#include "Runtime/Test/TestComponent.generated.h"

#include "Runtime/Test/TestComponent.h"
#include "Runtime/Core/DObject.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/ReflectionRegistry.h"

using namespace DeltaEngine;

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
}

static ReflectionRegistration registration_TestComponent(
    &Reflection::Private::ReflectionRegister_TestComponent::ReflectionRegisterFn_TestComponent);
