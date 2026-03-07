#pragma once

#include "EngineIncludes.h"

#include <cstdint>

DELTA_ENGINE_NS_BEGIN

struct BulkDataHandle
{
    uint32_t m_bulkId   = 0;
    uint64_t m_dataSize = 0;

    bool IsValid() const { return m_dataSize > 0; }

    bool operator==(const BulkDataHandle&) const = default;
};

DELTA_ENGINE_NS_END
