#pragma once

#include "EngineIncludes.h"
#include "Reflection/DProperty.h"
#include "Serialization/TBulkData.h"
#include "Serialization/AssetArchive.h"

#include <cstring>
#include <string>

DELTA_ENGINE_NS_BEGIN

class DBulkDataProperty : public DProperty
{
public:
    DBulkDataProperty(std::string name, uint32_t offset)
        : DProperty(std::move(name), "TBulkData", offset, sizeof(TBulkData))
    {
    }

    // ── DProperty pure virtuals ─────────────────────────────

    void InitializeValue(void* address) const override
    {
        new (address) TBulkData();
    }

    void DestroyValue(void* address) const override
    {
        static_cast<TBulkData*>(address)->~TBulkData();
    }

    void* GetValue(const void* instance) const override
    {
        void* addr = static_cast<uint8_t*>(const_cast<void*>(instance)) + m_offset;
        return addr;
    }

    void SetValue(void* instance, const void* field_value) const override
    {
        TBulkData& bulk = GetRef(instance);
        if (field_value)
        {
            const TBulkData& src = *static_cast<const TBulkData*>(field_value);
            bulk.Set(src.m_data, src.m_size);
        }
    }

    void CopyValue(void* dest, const void* src) const override
    {
        const TBulkData& srcBulk = *static_cast<const TBulkData*>(src);
        TBulkData* destBulk = new (dest) TBulkData();
        destBulk->Set(srcBulk.m_data, srcBulk.m_size);
    }

    bool Identical(const void* a, const void* b) const override
    {
        const TBulkData& ba = *static_cast<const TBulkData*>(a);
        const TBulkData& bb = *static_cast<const TBulkData*>(b);
        if (ba.m_size != bb.m_size) return false;
        if (ba.m_size == 0) return true;
        return std::memcmp(ba.m_data, bb.m_data, static_cast<size_t>(ba.m_size)) == 0;
    }

    std::string ToString(const void* address) const override
    {
        const TBulkData& bulk = *static_cast<const TBulkData*>(address);
        return "BulkData(" + std::to_string(bulk.m_size) + " bytes)";
    }

    EPropertyType GetPropertyType() const override
    {
        return EPropertyType::BulkData;
    }

    // ── Phase 1: serialize the handle (bulkId + size) into JSON ──

    void Serialize(AssetArchive& ar, void* objectPtr) override
    {
        TBulkData& bulk = GetRef(objectPtr);
        BulkDataHandle handle = bulk.ToHandle();
        ar.Serialize(GetName(), handle);
        if (ar.IsLoading())
            bulk.ApplyHandle(handle);
    }

    // ── Phase 2: read/write the raw binary payload ──────────

    void SerializeBulkPayload(AssetArchive& ar, void* objectPtr)
    {
        TBulkData& bulk = GetRef(objectPtr);
        if (ar.IsSaving())
        {
            if (bulk.IsValid())
                ar.WriteBulkData(bulk.m_bulkId, bulk.m_data, bulk.m_size);
        }
        else
        {
            if (bulk.m_size > 0)
            {
                auto bytes = ar.ReadBulkData(bulk.m_bulkId);
                bulk.Set(bytes.data(), static_cast<uint64_t>(bytes.size()));
            }
        }
    }

private:
    TBulkData& GetRef(void* objectPtr) const
    {
        return *static_cast<TBulkData*>(GetValue(objectPtr));
    }
};

DELTA_ENGINE_NS_END
