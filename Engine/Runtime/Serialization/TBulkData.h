#pragma once

#include "EngineIncludes.h"
#include "Serialization/BulkDataHandle.h"

#include <cstdint>
#include <cstring>

DELTA_ENGINE_NS_BEGIN

struct TBulkData
{
    /// Points to the loaded payload bytes, or `nullptr` when no payload is resident.
    uint8_t*  m_data   = nullptr;
    /// Stores the payload size in bytes.
    uint64_t  m_size   = 0;
    /// Stores the bulk sidecar identifier used during serialization.
    uint32_t  m_bulkId = 0;

    ~TBulkData()                           { delete[] m_data; }
    TBulkData()                            = default;

    TBulkData(const TBulkData& other)
    {
        m_size = other.m_size;
        m_bulkId = other.m_bulkId;
        if (other.m_data && other.m_size > 0)
        {
            m_data = new uint8_t[other.m_size];
            std::memcpy(m_data, other.m_data, other.m_size);
        }
    }

    TBulkData(TBulkData&& o) noexcept
        : m_data(o.m_data), m_size(o.m_size), m_bulkId(o.m_bulkId)
    {
        o.m_data = nullptr;
        o.m_size = 0;
        o.m_bulkId = 0;
    }

    TBulkData& operator=(TBulkData&& o) noexcept
    {
        if (this != &o)
        {
            delete[] m_data;
            m_data   = o.m_data;
            m_size   = o.m_size;
            m_bulkId = o.m_bulkId;
            o.m_data = nullptr;
            o.m_size = 0;
            o.m_bulkId = 0;
        }
        return *this;
    }

    /// Returns true when payload bytes are currently loaded.
    bool IsValid() const { return m_data != nullptr && m_size > 0; }

    /// Replaces the payload bytes with a deep copy of `src`.
    void Set(const uint8_t* src, uint64_t size)
    {
        delete[] m_data;
        m_data = nullptr;
        m_size = 0;
        if (src && size > 0)
        {
            m_data = new uint8_t[size];
            std::memcpy(m_data, src, size);
            m_size = size;
        }
    }

    /// Returns the serializable handle for the current bulk state.
    BulkDataHandle ToHandle()  const { return { m_bulkId, m_size }; }
    /// Applies a serialized handle without loading the payload bytes.
    void ApplyHandle(const BulkDataHandle& h) { m_bulkId = h.m_bulkId; m_size = h.m_dataSize; }
};

DELTA_ENGINE_NS_END
