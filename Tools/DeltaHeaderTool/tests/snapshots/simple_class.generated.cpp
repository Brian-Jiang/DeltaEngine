#include "simple_class.generated.h"

#include "fixtures/simple_class.h"


#include "Runtime/Reflection/DStruct.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/DFunction.h"
#include "Runtime/Reflection/ReflectionRegistry.h"

using namespace DeltaEngine;
using namespace DeltaEngine::Reflection::Private;

static void DoSomething_Thunk(DObject* instance, void* params)
{
    SimpleReflectClass_DoSomething_Params* typedParams = static_cast<SimpleReflectClass_DoSomething_Params*>(params);
    float x = typedParams->x;
    static_cast<SimpleReflectClass*>(instance)->DoSomething(x);
}

void Reflection::Private::ReflectionRegister_SimpleReflectClass::ReflectionRegisterFn_SimpleReflectClass()
{
    DClass* cls = new DClass("SimpleReflectClass",
                             "DObject",
                             sizeof(SimpleReflectClass),
                             alignof(SimpleReflectClass),
                             [](void* address) { new (address) SimpleReflectClass(); },
                             [](void* address) { static_cast<SimpleReflectClass*>(address)->~SimpleReflectClass(); },
                             [](void* dest, const void* src) { new (dest) SimpleReflectClass(*static_cast<const SimpleReflectClass*>(src)); },
                             nullptr
    );

    cls->AddProperty(new DFloatProperty(
        "myFloat",
        offsetof(SimpleReflectClass, myFloat)));

    cls->AddProperty(new DIntProperty(
        "myInt",
        offsetof(SimpleReflectClass, myInt)));

    {
        DFunction* fn = new DFunction("DoSomething", &DoSomething_Thunk, 1, sizeof(SimpleReflectClass_DoSomething_Params), 0);
        fn->AddParam(new DFloatProperty("x", offsetof(SimpleReflectClass_DoSomething_Params, x)));
        cls->AddFunction(fn);
    }

    GetReflectionRegistry().RegisterDClass(cls);
}

template <>
DELTAENGINE_API SimpleReflectClass* DeltaEngine::CreateDObject<SimpleReflectClass>()
{
    return GetReflectionRegistry().CreateObject<SimpleReflectClass>("SimpleReflectClass");
}

static ReflectionRegistration registration_SimpleReflectClass(
    &Reflection::Private::ReflectionRegister_SimpleReflectClass::ReflectionRegisterFn_SimpleReflectClass);
