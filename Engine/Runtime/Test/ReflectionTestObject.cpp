#include "Runtime/Test/ReflectionTestObject.h"

using namespace DeltaEngine;

void ReflectionTestObject::VoidMethod()
{
}

int ReflectionTestObject::Compute(int x)
{
    return x * 2;
}

float ReflectionTestObject::Compute(float x)
{
    return x + 1.0f;
}
