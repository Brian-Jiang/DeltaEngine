#pragma once

#include "EngineIncludes.h"

#include "Runtime/Core/DObject.h"
#include "Runtime/Core/GC/DObjectGCTypes.h"
#include "Runtime/Core/GC/DObjectRegistry.h"

DELTA_ENGINE_NS_BEGIN

/// Owning pointer that keeps its target alive by registering it as a GC root.
/// Resolution returns nullptr once the underlying slot has been freed/reused.
template <DObjectDerived T>
class StrongDObjectPtr
{
public:
    StrongDObjectPtr() = default;

    StrongDObjectPtr(T* object)
    {
        if (object)
        {
            m_handle = object->GetGCHandle();
            GetDObjectRegistry().AddRoot(m_handle);
        }
    }

    StrongDObjectPtr(const StrongDObjectPtr& other)
        : m_handle(other.m_handle)
    {
        if (m_handle.IsSet())
            GetDObjectRegistry().AddRoot(m_handle);
    }

    StrongDObjectPtr(StrongDObjectPtr&& other) noexcept
        : m_handle(other.m_handle)
    {
        other.m_handle = {};
    }

    StrongDObjectPtr& operator=(const StrongDObjectPtr& other)
    {
        if (this != &other)
        {
            ReleaseRoot();
            m_handle = other.m_handle;
            if (m_handle.IsSet())
                GetDObjectRegistry().AddRoot(m_handle);
        }
        return *this;
    }

    StrongDObjectPtr& operator=(StrongDObjectPtr&& other) noexcept
    {
        if (this != &other)
        {
            ReleaseRoot();
            m_handle       = other.m_handle;
            other.m_handle = {};
        }
        return *this;
    }

    ~StrongDObjectPtr() { ReleaseRoot(); }

    T* Get() const { return static_cast<T*>(GetDObjectRegistry().Resolve(m_handle)); }

    T*   operator->() const { return Get(); }
    T&   operator*() const { return *Get(); }
    explicit operator bool() const { return Get() != nullptr; }

    const DObjectHandle& GetHandle() const { return m_handle; }

    void Reset()
    {
        ReleaseRoot();
        m_handle = {};
    }

private:
    void ReleaseRoot()
    {
        if (m_handle.IsSet())
            GetDObjectRegistry().RemoveRoot(m_handle);
    }

    DObjectHandle m_handle;
};

DELTA_ENGINE_NS_END
