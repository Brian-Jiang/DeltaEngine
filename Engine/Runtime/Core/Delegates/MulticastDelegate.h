#pragma once

#include "Runtime/Core/Delegates/Delegate.h"
#include "Runtime/Core/Delegates/DelegateHandle.h"

#include <algorithm>
#include <utility>
#include <vector>

DELTA_ENGINE_NS_BEGIN

/// Multicast delegate with void signature; many subscribers, handle-based removal. Main-thread use only.
template<typename... Args>
class TMulticastDelegate<void(Args...)>
{
    struct FMulticastDelegateEntry
    {
        FDelegateHandle Handle;
        TDelegate<void(Args...)> Binding;
        const void* BoundObject = nullptr;
    };

public:
    TMulticastDelegate() = default;

    TMulticastDelegate(const TMulticastDelegate&) = default;
    TMulticastDelegate(TMulticastDelegate&&) noexcept = default;
    TMulticastDelegate& operator=(const TMulticastDelegate&) = default;
    TMulticastDelegate& operator=(TMulticastDelegate&&) noexcept = default;

    FDelegateHandle Add(TDelegate<void(Args...)>&& binding)
    {
        FMulticastDelegateEntry entry;
        entry.Handle = FDelegateHandle::Generate();
        entry.Binding = std::move(binding);
        m_entries.push_back(std::move(entry));
        return m_entries.back().Handle;
    }

    FDelegateHandle AddStatic(void (*func)(Args...))
    {
        TDelegate<void(Args...)> binding;
        binding.BindStatic(func);
        return Add(std::move(binding));
    }

    template<typename Functor>
    FDelegateHandle AddLambda(Functor&& functor)
    {
        TDelegate<void(Args...)> binding;
        binding.BindLambda(std::forward<Functor>(functor));
        return Add(std::move(binding));
    }

    template<typename UserClass>
    FDelegateHandle AddRaw(UserClass* object, void (UserClass::*method)(Args...))
    {
        TDelegate<void(Args...)> binding;
        binding.BindRaw(object, method);
        return AddRawBinding(std::move(binding), object);
    }

    template<typename UserClass>
    FDelegateHandle AddRaw(UserClass* object, void (UserClass::*method)(Args...) const)
    {
        TDelegate<void(Args...)> binding;
        binding.BindRaw(object, method);
        return AddRawBinding(std::move(binding), object);
    }

    template<typename UserClass>
    FDelegateHandle AddRaw(const UserClass* object, void (UserClass::*method)(Args...) const)
    {
        TDelegate<void(Args...)> binding;
        binding.BindRaw(object, method);
        return AddRawBinding(std::move(binding), object);
    }

    bool Remove(FDelegateHandle handle)
    {
        if (!handle.IsValid())
            return false;

        const auto it = std::find_if(m_entries.begin(), m_entries.end(),
            [&handle](const FMulticastDelegateEntry& entry)
            {
                return entry.Handle == handle;
            });

        if (it == m_entries.end())
            return false;

        m_entries.erase(it);
        return true;
    }

    size_t RemoveAll(const void* object)
    {
        if (!object)
            return 0;

        const auto newEnd = std::remove_if(m_entries.begin(), m_entries.end(),
            [object](const FMulticastDelegateEntry& entry)
            {
                return entry.BoundObject == object;
            });

        const size_t removedCount = static_cast<size_t>(std::distance(newEnd, m_entries.end()));
        m_entries.erase(newEnd, m_entries.end());
        return removedCount;
    }

    void Clear() { m_entries.clear(); }

    bool IsBound() const { return !m_entries.empty(); }

    /// Snapshot-at-start reentrancy: Add during broadcast runs on the next broadcast only;
    /// Remove/Clear during broadcast still runs removed listeners once from the snapshot.
    void Broadcast(Args... args) const
    {
        const std::vector<FMulticastDelegateEntry> snapshot = m_entries;

        for (const FMulticastDelegateEntry& entry : snapshot)
        {
            if (entry.Binding.IsBound())
                entry.Binding.Execute(std::forward<Args>(args)...);
        }
    }

private:
    FDelegateHandle AddRawBinding(TDelegate<void(Args...)>&& binding, const void* object)
    {
        FMulticastDelegateEntry entry;
        entry.Handle = FDelegateHandle::Generate();
        entry.Binding = std::move(binding);
        entry.BoundObject = object;
        m_entries.push_back(std::move(entry));
        return m_entries.back().Handle;
    }

    std::vector<FMulticastDelegateEntry> m_entries;
};

DELTA_ENGINE_NS_END
