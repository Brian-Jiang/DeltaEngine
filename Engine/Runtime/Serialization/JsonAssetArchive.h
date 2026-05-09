#pragma once

#include "EngineIncludes.h"

#include "Runtime/Serialization/AssetArchive.h"

#include <nlohmann/json.hpp>
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

DELTA_ENGINE_NS_BEGIN

class DELTAENGINE_API JsonAssetArchive : public AssetArchive
{
public:
    /// Creates a saving archive that only stores JSON payloads.
    explicit JsonAssetArchive();

    /// Creates a saving archive that can also write bulk-data sidecars.
    explicit JsonAssetArchive(const std::filesystem::path& assetDir, const std::string& assetName);

    /// Creates a loading archive from an existing JSON payload.
    explicit JsonAssetArchive(const nlohmann::json& root, const std::filesystem::path& assetDir);

    ~JsonAssetArchive() override = default;

    /// Returns the serialized JSON text.
    std::string         ToJsonString(int indent = 2) const;
    /// Returns the in-memory JSON document.
    const nlohmann::json& GetRoot() const;

    void        BeginObject(const std::string& className) override;
    std::string BeginObjectLoad()                         override;
    void        EndObject()                               override;

    void   BeginArray(const std::string& key, size_t count) override;
    size_t BeginArrayLoad(const std::string& key)           override;
    void   EndArray()                                        override;
    void   BeginNestedArray(size_t count)                    override;
    size_t BeginNestedArrayLoad()                            override;

    void BeginNestedObject(const std::string& key) override;
    bool BeginNestedObjectLoad(const std::string& key) override;
    void EndNestedObject() override;

    void Serialize(const std::string& key, float&        value) override;
    void Serialize(const std::string& key, double&       value) override;
    void Serialize(const std::string& key, int&          value) override;
    void Serialize(const std::string& key, bool&         value) override;
    void Serialize(const std::string& key, std::string&  value) override;

    void Serialize(const std::string& key, DirectX::SimpleMath::Vector3&    value) override;
    void Serialize(const std::string& key, DirectX::SimpleMath::Quaternion& value) override;
    void Serialize(const std::string& key, DirectX::XMFLOAT4&               value) override;
    void Serialize(const std::string& key, DirectX::XMFLOAT4X4&             value) override;
    void Serialize(const std::string& key, DirectX::BoundingBox&            value) override;

    void Serialize(const std::string& key, ScriptPointer&  value) override;
    void Serialize(const std::string& key, UUID&           value) override;
    void Serialize(const std::string& key, BulkDataHandle& value) override;
    void Serialize(const std::string& key, nlohmann::json& value) override;

    void SerializeElement(float&       value) override;
    void SerializeElement(double&      value) override;
    void SerializeElement(int&         value) override;
    void SerializeElement(bool&        value) override;
    void SerializeElement(std::string& value) override;
    void SerializeElement(DirectX::SimpleMath::Vector3&    value) override;
    void SerializeElement(DirectX::SimpleMath::Quaternion& value) override;
    void SerializeElement(DirectX::XMFLOAT4&               value) override;
    void SerializeElement(DirectX::XMFLOAT4X4&             value) override;
    void SerializeElement(DirectX::BoundingBox&            value) override;
    void SerializeElement(ScriptPointer&                    value) override;

    void                 WriteBulkData(uint32_t bulkId, const void* data, uint64_t size) override;
    std::vector<uint8_t> ReadBulkData(uint32_t bulkId)                                   override;

private:
    nlohmann::json               m_root;
    std::vector<nlohmann::json*> m_stack;
    std::vector<size_t>          m_arrayIndex;
    std::filesystem::path        m_assetDir;
    std::string                  m_assetName;
};

DELTA_ENGINE_NS_END
