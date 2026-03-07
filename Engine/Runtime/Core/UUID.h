#pragma once

#include "EngineIncludes.h"

#include <cstdint>
#include <string>
#include <compare>

DELTA_ENGINE_NS_BEGIN

struct DELTAENGINE_API UUID
{
    uint64_t m_high = 0;
    uint64_t m_low  = 0;

    static UUID Generate();
    static UUID Null();
    static UUID FromString(const std::string& str);

    std::string ToString() const;
    bool        IsNull() const { return m_high == 0 && m_low == 0; }

    bool operator==(const UUID&) const = default;
    auto operator<=>(const UUID&) const = default;
};

using AssetId  = UUID;
using ObjectId = UUID;

DELTA_ENGINE_NS_END

namespace std
{

template<>
struct hash<DeltaEngine::UUID>
{
    size_t operator()(const DeltaEngine::UUID& u) const
    {
        return hash<uint64_t>{}(u.m_high) ^ (hash<uint64_t>{}(u.m_low) << 1);
    }
};

}
