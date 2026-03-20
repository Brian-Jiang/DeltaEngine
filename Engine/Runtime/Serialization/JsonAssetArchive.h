#pragma once

#include "EngineIncludes.h"

#include "Serialization/AssetArchive.h"

#include <nlohmann/json.hpp>
#include <filesystem>
#include <vector>
#include <string>
#include <cstddef>

DELTA_ENGINE_NS_BEGIN

class DELTAENGINE_API JsonAssetArchive : public AssetArchive
{
public:
    // Saving constructor — no asset directory (bulk data is skipped)
    explicit JsonAssetArchive();

    // Saving constructor — with asset directory for bulk data sidecar writes
    explicit JsonAssetArchive(const std::filesystem::path& assetDir, const std::string& assetName);

    // Loading constructor
    explicit JsonAssetArchive(const nlohmann::json& root, const std::filesystem::path& assetDir);

    ~JsonAssetArchive() override = default;

    // --- Output accessors (Saving mode) ---
    std::string         ToJsonString(int indent = 2) const;
    const nlohmann::json& GetRoot() const;

    // --- Structural overrides ---

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

    // --- Primitive serialization overrides ---

    void Serialize(const std::string& key, float&        value) override;
    void Serialize(const std::string& key, double&       value) override;
    void Serialize(const std::string& key, int&          value) override;
    void Serialize(const std::string& key, bool&         value) override;
    void Serialize(const std::string& key, std::string&  value) override;
    void Serialize(const std::string& key, std::wstring& value) override;

    // --- Math type serialization overrides ---

    void Serialize(const std::string& key, DirectX::SimpleMath::Vector3&    value) override;
    void Serialize(const std::string& key, DirectX::SimpleMath::Quaternion& value) override;
    void Serialize(const std::string& key, DirectX::XMFLOAT4&               value) override;
    void Serialize(const std::string& key, DirectX::XMFLOAT4X4&             value) override;

    // --- Reference type serialization overrides ---

    void Serialize(const std::string& key, ScriptPointer&  value) override;
    void Serialize(const std::string& key, UUID&           value) override;
    void Serialize(const std::string& key, BulkDataHandle& value) override;

    // --- Array element serialization overrides ---

    void SerializeElement(float&       value) override;
    void SerializeElement(double&      value) override;
    void SerializeElement(int&         value) override;
    void SerializeElement(bool&        value) override;
    void SerializeElement(std::string& value) override;
    void SerializeElement(std::wstring& value) override;
    void SerializeElement(DirectX::SimpleMath::Vector3&    value) override;
    void SerializeElement(DirectX::SimpleMath::Quaternion& value) override;
    void SerializeElement(DirectX::XMFLOAT4&               value) override;
    void SerializeElement(DirectX::XMFLOAT4X4&             value) override;
    void SerializeElement(ScriptPointer&                    value) override;

    // --- Bulk data I/O overrides ---

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
