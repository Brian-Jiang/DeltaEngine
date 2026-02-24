#pragma once

#include "EngineIncludes.h"

#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Graphics/DXGraphicsContext.h"

#include "Runtime/Test/TestComponent.generated.h"


#define DCLASS(...)
#define DFUNCTION(...)
#define DPROPERTY(...)
#define DGENERATED_BODY(ClassName) \
    friend class DeltaEngine::Reflection::Private::ReflectionRegister_##ClassName;


DELTA_ENGINE_NS_BEGIN

class DTexture;

DCLASS()
class TestComponent : public DObject
{
    DGENERATED_BODY(TestComponent)

public:
    DFUNCTION()
    void TestFunction();


private:
    DPROPERTY()
    float m_testFloat;

    DPROPERTY()
    DTexture* m_texture;
};

DELTA_ENGINE_NS_END