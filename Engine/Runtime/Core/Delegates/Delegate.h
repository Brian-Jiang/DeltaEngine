#pragma once

#include "Runtime/Core/Delegates/DelegateFwd.h"

#include "Runtime/Core/DObject.h"
#include "Runtime/Core/GC/DObjectRegistry.h"

#include <functional>
#include <utility>

DELTA_ENGINE_NS_BEGIN

/// Single-cast delegate binding one callable. Main-thread use only.
template<typename Ret, typename... Args>
class TDelegate<Ret(Args...)>
{
public:
    TDelegate() = default;

    TDelegate(const TDelegate&) = default;
    TDelegate(TDelegate&&) noexcept = default;
    TDelegate& operator=(const TDelegate&) = default;
    TDelegate& operator=(TDelegate&&) noexcept = default;

    void BindStatic(Ret (*func)(Args...))
    {
        m_boundDObjectHandle = {};
        m_invocable = func;
    }

    template<typename Functor>
    void BindLambda(Functor&& functor)
    {
        m_boundDObjectHandle = {};
        m_invocable = std::forward<Functor>(functor);
    }

    template<typename UserClass>
    void BindRaw(UserClass* object, Ret (UserClass::*method)(Args...))
    {
        m_boundDObjectHandle = {};
        m_invocable = [object, method](Args... args) -> Ret
        {
            return (object->*method)(std::forward<Args>(args)...);
        };
    }

    template<typename UserClass>
    void BindRaw(UserClass* object, Ret (UserClass::*method)(Args...) const)
    {
        m_boundDObjectHandle = {};
        m_invocable = [object, method](Args... args) -> Ret
        {
            return (object->*method)(std::forward<Args>(args)...);
        };
    }

    template<typename UserClass>
    void BindRaw(const UserClass* object, Ret (UserClass::*method)(Args...) const)
    {
        m_boundDObjectHandle = {};
        m_invocable = [object, method](Args... args) -> Ret
        {
            return (object->*method)(std::forward<Args>(args)...);
        };
    }

    template<DObjectDerived UserClass>
    void BindDObject(UserClass* object, Ret (UserClass::*method)(Args...))
    {
        const DObjectHandle handle = object->GetGCHandle();
        m_boundDObjectHandle = handle;
        m_invocable = [handle, method](Args... args) -> Ret
        {
            UserClass* resolved = static_cast<UserClass*>(GetDObjectRegistry().Resolve(handle));
            if (!resolved)
                return Ret{};
            return (resolved->*method)(std::forward<Args>(args)...);
        };
    }

    template<DObjectDerived UserClass>
    void BindDObject(UserClass* object, Ret (UserClass::*method)(Args...) const)
    {
        const DObjectHandle handle = object->GetGCHandle();
        m_boundDObjectHandle = handle;
        m_invocable = [handle, method](Args... args) -> Ret
        {
            UserClass* resolved = static_cast<UserClass*>(GetDObjectRegistry().Resolve(handle));
            if (!resolved)
                return Ret{};
            return (resolved->*method)(std::forward<Args>(args)...);
        };
    }

    template<DObjectDerived UserClass>
    void BindDObject(const UserClass* object, Ret (UserClass::*method)(Args...) const)
    {
        const DObjectHandle handle = object->GetGCHandle();
        m_boundDObjectHandle = handle;
        m_invocable = [handle, method](Args... args) -> Ret
        {
            const UserClass* resolved = static_cast<const UserClass*>(GetDObjectRegistry().Resolve(handle));
            if (!resolved)
                return Ret{};
            return (resolved->*method)(std::forward<Args>(args)...);
        };
    }

    void Unbind()
    {
        m_invocable = nullptr;
        m_boundDObjectHandle = {};
    }

    bool IsBound() const
    {
        if (!static_cast<bool>(m_invocable))
            return false;
        if (m_boundDObjectHandle.IsSet())
            return GetDObjectRegistry().Resolve(m_boundDObjectHandle) != nullptr;
        return true;
    }

    Ret Execute(Args... args) const
    {
        DELTA_ASSERT(IsBound());
        return m_invocable(std::forward<Args>(args)...);
    }

    Ret ExecuteIfBound(Args... args) const
    {
        if (IsBound())
            return m_invocable(std::forward<Args>(args)...);
        return Ret{};
    }

private:
    std::function<Ret(Args...)> m_invocable;
    DObjectHandle               m_boundDObjectHandle{};
};

template<typename... Args>
class TDelegate<void(Args...)>
{
public:
    TDelegate() = default;

    TDelegate(const TDelegate&) = default;
    TDelegate(TDelegate&&) noexcept = default;
    TDelegate& operator=(const TDelegate&) = default;
    TDelegate& operator=(TDelegate&&) noexcept = default;

    void BindStatic(void (*func)(Args...))
    {
        m_boundDObjectHandle = {};
        m_invocable = func;
    }

    template<typename Functor>
    void BindLambda(Functor&& functor)
    {
        m_boundDObjectHandle = {};
        m_invocable = std::forward<Functor>(functor);
    }

    template<typename UserClass>
    void BindRaw(UserClass* object, void (UserClass::*method)(Args...))
    {
        m_boundDObjectHandle = {};
        m_invocable = [object, method](Args... args)
        {
            (object->*method)(std::forward<Args>(args)...);
        };
    }

    template<typename UserClass>
    void BindRaw(UserClass* object, void (UserClass::*method)(Args...) const)
    {
        m_boundDObjectHandle = {};
        m_invocable = [object, method](Args... args)
        {
            (object->*method)(std::forward<Args>(args)...);
        };
    }

    template<typename UserClass>
    void BindRaw(const UserClass* object, void (UserClass::*method)(Args...) const)
    {
        m_boundDObjectHandle = {};
        m_invocable = [object, method](Args... args)
        {
            (object->*method)(std::forward<Args>(args)...);
        };
    }

    template<DObjectDerived UserClass>
    void BindDObject(UserClass* object, void (UserClass::*method)(Args...))
    {
        const DObjectHandle handle = object->GetGCHandle();
        m_boundDObjectHandle = handle;
        m_invocable = [handle, method](Args... args)
        {
            UserClass* resolved = static_cast<UserClass*>(GetDObjectRegistry().Resolve(handle));
            if (!resolved)
                return;
            (resolved->*method)(std::forward<Args>(args)...);
        };
    }

    template<DObjectDerived UserClass>
    void BindDObject(UserClass* object, void (UserClass::*method)(Args...) const)
    {
        const DObjectHandle handle = object->GetGCHandle();
        m_boundDObjectHandle = handle;
        m_invocable = [handle, method](Args... args)
        {
            UserClass* resolved = static_cast<UserClass*>(GetDObjectRegistry().Resolve(handle));
            if (!resolved)
                return;
            (resolved->*method)(std::forward<Args>(args)...);
        };
    }

    template<DObjectDerived UserClass>
    void BindDObject(const UserClass* object, void (UserClass::*method)(Args...) const)
    {
        const DObjectHandle handle = object->GetGCHandle();
        m_boundDObjectHandle = handle;
        m_invocable = [handle, method](Args... args)
        {
            const UserClass* resolved = static_cast<const UserClass*>(GetDObjectRegistry().Resolve(handle));
            if (!resolved)
                return;
            (resolved->*method)(std::forward<Args>(args)...);
        };
    }

    void Unbind()
    {
        m_invocable = nullptr;
        m_boundDObjectHandle = {};
    }

    bool IsBound() const
    {
        if (!static_cast<bool>(m_invocable))
            return false;
        if (m_boundDObjectHandle.IsSet())
            return GetDObjectRegistry().Resolve(m_boundDObjectHandle) != nullptr;
        return true;
    }

    void Execute(Args... args) const
    {
        DELTA_ASSERT(IsBound());
        m_invocable(std::forward<Args>(args)...);
    }

    void ExecuteIfBound(Args... args) const
    {
        if (IsBound())
            m_invocable(std::forward<Args>(args)...);
    }

private:
    std::function<void(Args...)> m_invocable;
    DObjectHandle                m_boundDObjectHandle{};
};

DELTA_ENGINE_NS_END
