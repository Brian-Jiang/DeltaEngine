#pragma once

#include "EngineIncludes.h"

DELTA_ENGINE_NS_BEGIN

class TestComponent;

namespace Reflection {
namespace Private {

struct TestComponent_TestAdd_Params
{
    int a;
    int b;
    int returnValue;
};

struct TestComponent_TestMultiply_Params
{
    float x;
    bool neg;
    float returnValue;
};

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
