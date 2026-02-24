#pragma once

#include "EngineIncludes.h"

#include <memory>

#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Graphics/DXGraphicsContext.h"

#define DCLASS(...)
#define DFUNCTION(...)
#define DPROPERTY(...)
#define DGENERATED_BODY(ClassName) \
    friend class DeltaEngine::Reflection::Private::ReflectionRegister_##ClassName; \

//class DeltaEngine::Reflection::Private::ReflectionRegister_TestComponent;

namespace DeltaEngine::Reflection::Private
{
    class ReflectionRegister_TestComponent;
}

DELTA_ENGINE_NS_BEGIN

class DTexture;

DCLASS()
class TestComponent : public DObject
{
    DGENERATED_BODY(TestComponent)
    //friend class DeltaEngine::Reflection::Private::ReflectionRegister_TestComponent;

    friend class DWorld;

public:
    DFUNCTION()
    void TestFunction();


private:
    DPROPERTY()
    float m_testFloat;

    DPROPERTY()
    std::shared_ptr<DTexture> m_texture;

public:

    // =======================================

    //static const char* GetClassName_TestComponent() { return "TestComponent"; }

    //static const char* GetFieldName_m_testFloat() { return "m_testFloat"; }
    //static const char* GetFieldType_m_testFloat() { return "float"; }
    //static void SetField_m_testFloat(void* instance, std::shared_ptr<void> field_value) { static_cast<TestComponent*>(instance)->m_testFloat = *std::static_pointer_cast<float>(field_value); }
    //static void* GetField_m_testFloat(void* instance) { return static_cast<void*>(&(static_cast<TestComponent*>(instance)->m_testFloat)); }

    //static const char* GetFieldName_m_texture() { return "m_texture"; }
    //static const char* GetFieldType_m_texture() { return "std::shared_ptr<DTexture>"; }
    //static void SetField_m_texture(void* instance, std::shared_ptr<void> field_value) { static_cast<TestComponent*>(instance)->m_texture = std::static_pointer_cast<DTexture>(field_value); }
    //static void* GetField_m_texture(void* instance) { return static_cast<void*>(&(static_cast<TestComponent*>(instance)->m_texture)); }
};

DELTA_ENGINE_NS_END