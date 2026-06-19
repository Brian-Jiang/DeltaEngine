#include "Runtime/Test/TestComponent.h"

using namespace DeltaEngine;

void TestComponent::TestFunction()
{
    ++m_testInt;
    DLOG(LogCore, ELogLevel::Log, "Test component log");
}

int TestComponent::TestAdd(int a, int b)
{
    return a + b;
}

float TestComponent::TestMultiply(float x, bool negate)
{
    return negate ? -x : x;
}
