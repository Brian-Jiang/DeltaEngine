#pragma once

#include "EngineIncludes.h"
#include "Core/DHandle.h"
#include "Reflection/ReflectionRegistry.h"

#include "DObject.generated.h"

DELTA_ENGINE_NS_BEGIN

class DClass;

DCLASS()
class DObject
{
    friend class DeltaEngine::Reflection::Private::ReflectionRegister_DObject;

public:
    virtual DClass* GetClass() const
    {
        return GetReflectionRegistry().FindClassByName("DObject");
    }

    friend class DClass;

public:
    DELTAENGINE_API DObject();
    DELTAENGINE_API virtual ~DObject();

    void SetHandle(const DHandle& handle) { m_handle = handle; }

private:
    DHandle m_handle;

};

template <typename T>
concept DObjectDerived = std::derived_from<T, DObject>;

DELTA_ENGINE_NS_END