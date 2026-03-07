#pragma once

#include "EngineIncludes.h"

#include "Core/UUID.h"
#include "Serialization/ScriptPointer.h"
#include "Serialization/BulkDataHandle.h"

#include "SimpleMath.h"
#include <DirectXMath.h>

#include <string>
#include <vector>
#include <cstdint>

DELTA_ENGINE_NS_BEGIN

class DELTAENGINE_API AssetArchive
{
public:
    enum class Mode { Loading, Saving };

    explicit AssetArchive(Mode mode) : m_mode(mode) {}
    virtual ~AssetArchive() = default;

    AssetArchive(const AssetArchive&)            = delete;
    AssetArchive& operator=(const AssetArchive&) = delete;

    Mode GetMode()   const { return m_mode; }
    bool IsLoading() const { return m_mode == Mode::Loading; }
    bool IsSaving()  const { return m_mode == Mode::Saving; }

    // --- Structural operations ---

    virtual void        BeginObject(const std::string& className) = 0;
    virtual std::string BeginObjectLoad()                         = 0;
    virtual void        EndObject()                               = 0;

    virtual void   BeginArray(const std::string& key, size_t count) = 0;
    virtual size_t BeginArrayLoad(const std::string& key)           = 0;
    virtual void   EndArray()                                        = 0;

    // --- Primitive serialization ---

    virtual void Serialize(const std::string& key, float&       value) = 0;
    virtual void Serialize(const std::string& key, double&      value) = 0;
    virtual void Serialize(const std::string& key, int&         value) = 0;
    virtual void Serialize(const std::string& key, bool&        value) = 0;
    virtual void Serialize(const std::string& key, std::string& value) = 0;
    virtual void Serialize(const std::string& key, std::wstring& value) = 0;

    // --- Math type serialization ---

    virtual void Serialize(const std::string& key, DirectX::SimpleMath::Vector3&    value) = 0;
    virtual void Serialize(const std::string& key, DirectX::SimpleMath::Quaternion& value) = 0;
    virtual void Serialize(const std::string& key, DirectX::XMFLOAT4&               value) = 0;
    virtual void Serialize(const std::string& key, DirectX::XMFLOAT4X4&             value) = 0;

    // --- Reference type serialization ---

    virtual void Serialize(const std::string& key, ScriptPointer&  value) = 0;
    virtual void Serialize(const std::string& key, UUID&           value) = 0;
    virtual void Serialize(const std::string& key, BulkDataHandle& value) = 0;

    // --- Bulk data I/O ---

    virtual void                  WriteBulkData(uint32_t bulkId, const void* data, uint64_t size) = 0;
    virtual std::vector<uint8_t>  ReadBulkData(uint32_t bulkId)                                   = 0;

protected:
    Mode m_mode;
};

DELTA_ENGINE_NS_END
