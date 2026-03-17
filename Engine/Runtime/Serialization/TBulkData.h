#pragma once

#include "EngineIncludes.h"
#include "Serialization/BulkDataHandle.h"

#include <cstdint>
#include <cstring>

DELTA_ENGINE_NS_BEGIN

struct TBulkData
{
    uint8_t*  m_data   = nullptr;
    uint64_t  m_size   = 0;
    uint32_t  m_bulkId = 0;

    ~TBulkData()                           { delete[] m_data; }
    TBulkData()                            = default;
    TBulkData(const TBulkData&)            = delete;
    TBulkData& operator=(const TBulkData&) = delete;

    TBulkData(TBulkData&& o) noexcept
        : m_data(o.m_data), m_size(o.m_size), m_bulkId(o.m_bulkId)
    {
        o.m_data = nullptr;
        o.m_size = 0;
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
        }
        return *this;
    }

    bool IsValid() const { return m_data != nullptr && m_size > 0; }

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

    BulkDataHandle ToHandle()  const { return { m_bulkId, m_size }; }
    void ApplyHandle(const BulkDataHandle& h) { m_bulkId = h.m_bulkId; m_size = h.m_dataSize; }
};

DELTA_ENGINE_NS_END
