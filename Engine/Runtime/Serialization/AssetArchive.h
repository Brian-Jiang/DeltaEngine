#pragma once

#include "EngineIncludes.h"

#include "Runtime/Core/UUID.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Serialization/BulkDataHandle.h"
#include "Runtime/Serialization/ScriptPointer.h"

#include "SimpleMath.h"
#include <DirectXCollision.h>
#include <DirectXMath.h>

#include <cstdint>
#include <exception>
#include <string>
#include <vector>

DELTA_ENGINE_NS_BEGIN

class DELTAENGINE_API AssetArchive
{
public:
    /// Identifies whether the archive reads from or writes to its backing store.
    enum class Mode { Loading, Saving };

    explicit AssetArchive(Mode mode) : m_mode(mode) {}
    virtual ~AssetArchive() = default;

    AssetArchive(const AssetArchive&)            = delete;
    AssetArchive& operator=(const AssetArchive&) = delete;

    Mode GetMode()   const { return m_mode; }
    bool IsLoading() const { return m_mode == Mode::Loading; }
    bool IsSaving()  const { return m_mode == Mode::Saving; }

    /// Begins an object scope for the given reflected class name.
    virtual void        BeginObject(const std::string& className) = 0;
    /// Begins loading an object scope and returns its stored class name.
    virtual std::string BeginObjectLoad()                         = 0;
    /// Ends the current object scope.
    virtual void        EndObject()                               = 0;

    /// Begins a named array scope with the provided element count.
    virtual void   BeginArray(const std::string& key, size_t count) = 0;
    /// Begins loading a named array scope and returns its element count.
    virtual size_t BeginArrayLoad(const std::string& key)           = 0;
    /// Ends the current array scope.
    virtual void   EndArray()                                        = 0;

    /// Begins an unnamed nested array scope for an array element.
    virtual void   BeginNestedArray(size_t count) = 0;
    /// Begins loading an unnamed nested array scope and returns its element count.
    virtual size_t BeginNestedArrayLoad()         = 0;

    /// Begins a named nested object scope when the archive format supports it.
    virtual void BeginNestedObject(const std::string& key) {}
    /// Begins loading a named nested object scope when present.
    virtual bool BeginNestedObjectLoad(const std::string& key) { return false; }
    /// Ends the current nested object scope.
    virtual void EndNestedObject() {}

    virtual void Serialize(const std::string& key, float&       value) = 0;
    virtual void Serialize(const std::string& key, double&      value) = 0;
    virtual void Serialize(const std::string& key, int&         value) = 0;
    virtual void Serialize(const std::string& key, bool&        value) = 0;
    virtual void Serialize(const std::string& key, std::string& value) = 0;

    virtual void Serialize(const std::string& key, DirectX::SimpleMath::Vector3&    value) = 0;
    virtual void Serialize(const std::string& key, DirectX::SimpleMath::Quaternion& value) = 0;
    virtual void Serialize(const std::string& key, DirectX::XMFLOAT4&               value) = 0;
    virtual void Serialize(const std::string& key, DirectX::XMFLOAT4X4&             value) = 0;
    virtual void Serialize(const std::string& key, DirectX::BoundingBox&            value) = 0;

    virtual void Serialize(const std::string& key, ScriptPointer&  value) = 0;
    virtual void Serialize(const std::string& key, UUID&           value) = 0;
    virtual void Serialize(const std::string& key, BulkDataHandle& value) = 0;

    /// Serializes the next array element in the current array scope.
    virtual void SerializeElement(float&       value) = 0;
    virtual void SerializeElement(double&      value) = 0;
    virtual void SerializeElement(int&         value) = 0;
    virtual void SerializeElement(bool&        value) = 0;
    virtual void SerializeElement(std::string& value) = 0;
    virtual void SerializeElement(DirectX::SimpleMath::Vector3&    value) = 0;
    virtual void SerializeElement(DirectX::SimpleMath::Quaternion& value) = 0;
    virtual void SerializeElement(DirectX::XMFLOAT4&               value) = 0;
    virtual void SerializeElement(DirectX::XMFLOAT4X4&             value) = 0;
    virtual void SerializeElement(DirectX::BoundingBox&            value) = 0;
    virtual void SerializeElement(ScriptPointer&                    value) = 0;

    /// Serializes a named reflected vector using its inner property serializer.
    template <typename T>
    void Serialize(const std::string& key, std::vector<T>& vec, DProperty& innerProp)
    {
        if (IsSaving())
        {
            BeginArray(key, vec.size());
            for (size_t i = 0; i < vec.size(); ++i)
                innerProp.SerializeElement(*this, &vec[i]);
            EndArray();
        }
        else
        {
            size_t count = BeginArrayLoad(key);
            try
            {
                vec.resize(count);
            }
            catch (const std::exception& ex)
            {
                DLOG(LogSerialization, ELogLevel::Error,
                     "Failed to resize serialized array '{}': requested {} elements but allocation failed ({})",
                     key, count, ex.what());
                EndArray();
                return;
            }
            for (size_t i = 0; i < count; ++i)
                innerProp.SerializeElement(*this, &vec[i]);
            EndArray();
        }
    }

    /// Serializes a nested reflected vector stored inside another array.
    template <typename T>
    void SerializeNested(std::vector<T>& vec, DProperty& innerProp)
    {
        if (IsSaving())
        {
            BeginNestedArray(vec.size());
            for (size_t i = 0; i < vec.size(); ++i)
                innerProp.SerializeElement(*this, &vec[i]);
            EndArray();
        }
        else
        {
            size_t count = BeginNestedArrayLoad();
            try
            {
                vec.resize(count);
            }
            catch (const std::exception& ex)
            {
                DLOG(LogSerialization, ELogLevel::Error,
                     "Failed to resize nested serialized array: requested {} elements but allocation failed ({})",
                     count, ex.what());
                EndArray();
                return;
            }
            for (size_t i = 0; i < count; ++i)
                innerProp.SerializeElement(*this, &vec[i]);
            EndArray();
        }
    }

    /// Writes a binary bulk payload for the given bulk identifier.
    virtual void                  WriteBulkData(uint32_t bulkId, const void* data, uint64_t size) = 0;
    /// Reads a binary bulk payload for the given bulk identifier.
    virtual std::vector<uint8_t>  ReadBulkData(uint32_t bulkId)                                   = 0;

protected:
    Mode m_mode;
};

DELTA_ENGINE_NS_END
