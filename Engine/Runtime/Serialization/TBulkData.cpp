#include "Runtime/Serialization/TBulkData.h"

#include <cstring>
#include <new>

using namespace DeltaEngine;

TBulkData::~TBulkData()
{
    delete[] m_data;
}

TBulkData::TBulkData() = default;

TBulkData::TBulkData(const TBulkData& other)
{
    m_size = other.m_size;
    m_bulkId = other.m_bulkId;
    if (other.m_data && other.m_size > 0)
    {
        m_data = new (std::nothrow) uint8_t[other.m_size];
        if (!m_data)
        {
            DLOG(LogSerialization, ELogLevel::Error,
                 "Failed to copy bulk data payload: allocation failed for {} bytes (bulkId: {})",
                 other.m_size, other.m_bulkId);
            m_size = 0;
            return;
        }
        std::memcpy(m_data, other.m_data, other.m_size);
    }
}

TBulkData::TBulkData(TBulkData&& o) noexcept
    : m_data(o.m_data), m_size(o.m_size), m_bulkId(o.m_bulkId)
{
    o.m_data = nullptr;
    o.m_size = 0;
    o.m_bulkId = 0;
}

TBulkData& TBulkData::operator=(TBulkData&& o) noexcept
{
    if (this != &o)
    {
        delete[] m_data;
        m_data = o.m_data;
        m_size = o.m_size;
        m_bulkId = o.m_bulkId;
        o.m_data = nullptr;
        o.m_size = 0;
        o.m_bulkId = 0;
    }
    return *this;
}

bool TBulkData::IsValid() const
{
    return m_data != nullptr && m_size > 0;
}

void TBulkData::Set(const uint8_t* src, uint64_t size)
{
    delete[] m_data;
    m_data = nullptr;
    m_size = 0;
    if (size > 0 && !src)
    {
        DLOG(LogSerialization, ELogLevel::Warning,
             "Skipped setting bulk data payload: source is null but size is {} bytes (bulkId: {})",
             size, m_bulkId);
        DELTA_ENSURE_MSG(false, "TBulkData::Set received null source with nonzero size");
        return;
    }
    if (src && size > 0)
    {
        m_data = new (std::nothrow) uint8_t[size];
        if (!m_data)
        {
            DLOG(LogSerialization, ELogLevel::Error,
                 "Failed to set bulk data payload: allocation failed for {} bytes (bulkId: {})",
                 size, m_bulkId);
            return;
        }
        std::memcpy(m_data, src, size);
        m_size = size;
    }
}

BulkDataHandle TBulkData::ToHandle() const
{
    return { m_bulkId, m_size };
}

void TBulkData::ApplyHandle(const BulkDataHandle& h)
{
    delete[] m_data;
    m_data = nullptr;
    m_bulkId = h.m_bulkId;
    m_size = h.m_dataSize;
}
