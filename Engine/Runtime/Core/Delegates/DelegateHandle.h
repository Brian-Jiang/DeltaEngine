#pragma once

#include "EngineIncludes.h"

#include <cstdint>

DELTA_ENGINE_NS_BEGIN

/// Opaque handle identifying a single binding in a multicast delegate. Main-thread use only.
struct FDelegateHandle
{
    uint64_t m_id = 0;

    bool IsValid() const { return m_id != 0; }

    bool operator==(const FDelegateHandle&) const = default;

    static FDelegateHandle Generate()
    {
        static uint64_t s_nextId = 1;
        return FDelegateHandle{s_nextId++};
    }
};

DELTA_ENGINE_NS_END
