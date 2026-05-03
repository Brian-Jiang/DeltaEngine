#pragma once

#include "EngineIncludes.h"

#include "Runtime/Serialization/BulkDataHandle.h"

#include <cstdint>

DELTA_ENGINE_NS_BEGIN

struct TBulkData
{
    /// Points to the loaded payload bytes, or `nullptr` when no payload is resident.
    uint8_t*  m_data   = nullptr;
    /// Stores the payload size in bytes.
    uint64_t  m_size   = 0;
    /// Stores the bulk sidecar identifier used during serialization.
    uint32_t  m_bulkId = 0;

    DELTAENGINE_API ~TBulkData();
    DELTAENGINE_API TBulkData();

    DELTAENGINE_API TBulkData(const TBulkData& other);

    DELTAENGINE_API TBulkData(TBulkData&& o) noexcept;

    DELTAENGINE_API TBulkData& operator=(TBulkData&& o) noexcept;

    /// Returns true when payload bytes are currently loaded.
    DELTAENGINE_API bool IsValid() const;

    /// Replaces the payload bytes with a deep copy of `src`.
    DELTAENGINE_API void Set(const uint8_t* src, uint64_t size);

    /// Returns the serializable handle for the current bulk state.
    DELTAENGINE_API BulkDataHandle ToHandle() const;
    /// Applies a serialized handle without loading the payload bytes.
    DELTAENGINE_API void ApplyHandle(const BulkDataHandle& h);
};

DELTA_ENGINE_NS_END
