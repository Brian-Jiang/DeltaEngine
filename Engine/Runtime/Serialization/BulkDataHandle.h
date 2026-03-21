#pragma once

#include "EngineIncludes.h"

#include <cstdint>

DELTA_ENGINE_NS_BEGIN

struct BulkDataHandle
{
    /// Identifies the bulk sidecar entry.
    uint32_t m_bulkId   = 0;
    /// Stores the expected payload size in bytes.
    uint64_t m_dataSize = 0;

    /// Returns true when the handle describes a non-empty payload.
    bool IsValid() const { return m_dataSize > 0; }

    bool operator==(const BulkDataHandle&) const = default;
};

DELTA_ENGINE_NS_END
