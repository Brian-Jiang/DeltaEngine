#pragma once

#include "EngineIncludes.h"

#include "Runtime/Core/DObject.h"
#include "Runtime/Test/TestComponent.h"

#include "TestComponent2.generated.h"

DELTA_ENGINE_NS_BEGIN

DCLASS()
class TestComponent2 : public TestComponent
{
    DGENERATED_BODY(TestComponent2)
};

DELTA_ENGINE_NS_END