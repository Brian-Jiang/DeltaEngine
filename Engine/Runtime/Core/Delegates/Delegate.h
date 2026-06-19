#pragma once

#include "Runtime/Core/Delegates/DelegateFwd.h"

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
        m_invocable = func;
    }

    template<typename Functor>
    void BindLambda(Functor&& functor)
    {
        m_invocable = std::forward<Functor>(functor);
    }

    template<typename UserClass>
    void BindRaw(UserClass* object, Ret (UserClass::*method)(Args...))
    {
        m_invocable = [object, method](Args... args) -> Ret
        {
            return (object->*method)(std::forward<Args>(args)...);
        };
    }

    template<typename UserClass>
    void BindRaw(UserClass* object, Ret (UserClass::*method)(Args...) const)
    {
        m_invocable = [object, method](Args... args) -> Ret
        {
            return (object->*method)(std::forward<Args>(args)...);
        };
    }

    void Unbind() { m_invocable = nullptr; }

    bool IsBound() const { return static_cast<bool>(m_invocable); }

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
        m_invocable = func;
    }

    template<typename Functor>
    void BindLambda(Functor&& functor)
    {
        m_invocable = std::forward<Functor>(functor);
    }

    template<typename UserClass>
    void BindRaw(UserClass* object, void (UserClass::*method)(Args...))
    {
        m_invocable = [object, method](Args... args)
        {
            (object->*method)(std::forward<Args>(args)...);
        };
    }

    template<typename UserClass>
    void BindRaw(UserClass* object, void (UserClass::*method)(Args...) const)
    {
        m_invocable = [object, method](Args... args)
        {
            (object->*method)(std::forward<Args>(args)...);
        };
    }

    void Unbind() { m_invocable = nullptr; }

    bool IsBound() const { return static_cast<bool>(m_invocable); }

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
};

DELTA_ENGINE_NS_END
