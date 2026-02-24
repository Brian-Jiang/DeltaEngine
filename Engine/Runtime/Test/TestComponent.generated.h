#pragma once

#include "EngineIncludes.h"

#include "Runtime/Test/TestComponent.h"
#include "Runtime/Core/DObject.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/ReflectionRegistry.h"

DELTA_ENGINE_NS_BEGIN

class DTexture;

namespace Reflection
{
namespace Private
{

//void SetField_m_testFloat(void* instance, std::shared_ptr<void> field_value) { static_cast<TestComponent*>(instance)->m_testFloat = *std::static_pointer_cast<float>(field_value); }
//    void* GetField_m_testFloat(void* instance) { return static_cast<void*>(&(static_cast<TestComponent*>(instance)->m_testFloat)); }

class ReflectionRegister_TestComponent
{
public:

    static void ReflectionRegisterFn_TestComponent()
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

        //cls->m_name = "TestComponent";
        //cls->m_super = nullptr; // Assuming no inheritance for simplicity
        //cls->m_properties = nullptr; // Properties will be added later
        //cls->m_ownProperties = nullptr; // Own properties will be added later
        //cls->m_classSize = sizeof(TestComponent);
        //cls->m_minAlignment = alignof(TestComponent);
        //cls->m_constructFn = [](void* address) { new (address) TestComponent(); };
        //cls->m_destructFn = [](void* address) { static_cast<TestComponent*>(address)->~TestComponent(); };
        //cls->m_copyFn = [](void* dest, const void* src) { new (dest) TestComponent(*static_cast<const TestComponent*>(src)); };
        //cls->m_classDefaultObject = nullptr; // Default object can be set if needed

        //#define _CRT_USE_BUILTIN_OFFSETOF
    
        // Register properties of TestComponent
    
        DProperty* prop_m_testFloat = new DFloatProperty("m_testFloat",
                                                         offsetof(TestComponent, m_testFloat));

        DProperty* prop_m_texture = new DObjectPtrProperty<DTexture>("m_texture",
                                                                     "DTexture",
                                                        offsetof(TestComponent, m_texture));

        //prop_m_testFloat->m_name = "m_testFloat";
        //prop_m_testFloat->m_type = "float";
        //prop_m_testFloat->m_offset = offsetof(TestComponent, m_testFloat);
        //prop_m_testFloat->m_size = sizeof(float);
        //prop_m_testFloat->m_setter = &TestComponent::SetField_m_testFloat;
        //prop_m_testFloat->m_getter = &TestComponent::GetField_m_testFloat;
        //prop_m_testFloat->m_next = nullptr; // Assuming it's the first property

        //DProperty* prop_m_texture = new DProperty();
        //prop_m_texture->m_name = "m_texture";
        //prop_m_texture->m_type = "std::shared_ptr<DTexture>";
        //prop_m_texture->m_offset = offsetof(TestComponent, m_texture);
        //prop_m_texture->m_size = sizeof(std::shared_ptr<DTexture>);
        //prop_m_texture->m_setter = &TestComponent::SetField_m_texture;
        //prop_m_texture->m_getter = &TestComponent::GetField_m_texture;
        //prop_m_texture->m_next = nullptr; // Assuming it's the second property

        // Link properties to the class
        cls->AddProperty(prop_m_testFloat);
        cls->AddProperty(prop_m_texture);

        // Register the class in the reflection registry
        //ReflectionRegistry::Get().RegisterClass(cls);
    }

};

static ReflectionRegistration registration_TestComponent(&ReflectionRegister_TestComponent::ReflectionRegisterFn_TestComponent);

}
}


DELTA_ENGINE_NS_END