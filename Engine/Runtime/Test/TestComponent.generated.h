#pragma once

#include "EngineIncludes.h"

#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Core/DObject.h"

DELTA_ENGINE_NS_BEGIN

class TestComponent;

namespace Reflection {
namespace Private {

class ReflectionRegister_TestComponent
{
public:
    static void ReflectionRegisterFn_TestComponent();
};

} // namespace Private
} // namespace Reflection


template <>
TestComponent* CreateDObject<TestComponent>();

DELTA_ENGINE_NS_END