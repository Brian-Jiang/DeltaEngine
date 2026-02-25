#include "Runtime/Test/TestComponent.h"

using namespace DeltaEngine;

void TestComponent::TestFunction()
{
}

int TestComponent::TestAdd(int a, int b)
{
    return a + b;
}

float TestComponent::TestMultiply(float x, bool negate)
{
    return negate ? -x : x;
}