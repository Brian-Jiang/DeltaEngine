#pragma once

#include "EngineIncludes.h"

#include "Runtime/Core/DObject.h"
#include "Runtime/Core/GC/DObjectGCTypes.h"
#include "Runtime/Core/GC/DObjectRegistry.h"
#include "Runtime/Core/GC/StrongDObjectPtr.h"

DELTA_ENGINE_NS_BEGIN

/// Non-owning pointer that auto-nulls once its target slot is freed (version bump).
template <DObjectDerived T>
class WeakDObjectPtr
{
public:
    WeakDObjectPtr() = default;

    WeakDObjectPtr(T* object)
    {
        if (object)
            m_handle = object->GetGCHandle();
    }

    WeakDObjectPtr(const StrongDObjectPtr<T>& strong)
        : m_handle(strong.GetHandle())
    {
    }

    T* Get() const { return static_cast<T*>(GetDObjectRegistry().Resolve(m_handle)); }

    T*   operator->() const { return Get(); }
    T&   operator*() const { return *Get(); }
    explicit operator bool() const { return Get() != nullptr; }

    bool IsValid() const { return GetDObjectRegistry().IsValid(m_handle); }

    const DObjectHandle& GetHandle() const { return m_handle; }

    void Reset() { m_handle = {}; }

private:
    DObjectHandle m_handle;
};

DELTA_ENGINE_NS_END
