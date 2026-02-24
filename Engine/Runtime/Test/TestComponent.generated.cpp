#include "Runtime/Test/TestComponent.generated.h"

#include "Runtime/Test/TestComponent.h"
#include "Runtime/Core/DObject.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/ReflectionRegistry.h"

using namespace DeltaEngine;

void Reflection::Private::ReflectionRegister_TestComponent::ReflectionRegisterFn_TestComponent()
{
    // Register class TestComponent
    DClass* cls = new DClass("TestComponent",
                             nullptr, // No superclass
                             sizeof(TestComponent),
                             alignof(TestComponent),
                             [](void* address) { new (address) TestComponent(); },
                             [](void* address) { static_cast<TestComponent*>(address)->~TestComponent(); },
                             [](void* dest, const void* src) { new (dest) TestComponent(*static_cast<const TestComponent*>(src)); },
                             nullptr // No default object
    );

    // Register properties of TestComponent

    DProperty* prop_m_testFloat = new DFloatProperty("m_testFloat",
        offsetof(TestComponent, m_testFloat));

    DProperty* prop_m_texture = new DObjectPtrProperty<DTexture>("m_texture",
        "DTexture",
        offsetof(TestComponent, m_texture));

    // Link properties to the class
    cls->AddProperty(prop_m_testFloat);
    cls->AddProperty(prop_m_texture);
}

static ReflectionRegistration registration_TestComponent(&Reflection::Private::ReflectionRegister_TestComponent::ReflectionRegisterFn_TestComponent);
