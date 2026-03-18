#include "Serialization/JsonAssetArchive.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <fstream>
#include <stdexcept>

using namespace DeltaEngine;

namespace DeltaEngine
{

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static std::string WStringToUtf8(const std::wstring& wide)
{
    if (wide.empty())
        return {};

    const int size = WideCharToMultiByte(
        CP_UTF8, 0,
        wide.data(), static_cast<int>(wide.size()),
        nullptr, 0,
        nullptr, nullptr);

    if (size <= 0)
        return {};

    std::string result(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(
        CP_UTF8, 0,
        wide.data(), static_cast<int>(wide.size()),
        result.data(), size,
        nullptr, nullptr);
    return result;
}

static std::wstring Utf8ToWString(const std::string& utf8)
{
    if (utf8.empty())
        return {};

    const int size = MultiByteToWideChar(
        CP_UTF8, 0,
        utf8.data(), static_cast<int>(utf8.size()),
        nullptr, 0);

    if (size <= 0)
        return {};

    std::wstring result(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(
        CP_UTF8, 0,
        utf8.data(), static_cast<int>(utf8.size()),
        result.data(), size);
    return result;
}

}

// ---------------------------------------------------------------------------
// Constructors
// ---------------------------------------------------------------------------

JsonAssetArchive::JsonAssetArchive()
    : AssetArchive(Mode::Saving)
    , m_root(nlohmann::json::object())
{
    m_stack.push_back(&m_root);
}

JsonAssetArchive::JsonAssetArchive(const std::filesystem::path& assetDir, const std::string& assetName)
    : AssetArchive(Mode::Saving)
    , m_root(nlohmann::json::object())
    , m_assetDir(assetDir)
    , m_assetName(assetName)
{
    m_stack.push_back(&m_root);
}

JsonAssetArchive::JsonAssetArchive(const nlohmann::json& root, const std::filesystem::path& assetDir)
    : AssetArchive(Mode::Loading)
    , m_root(root)
    , m_assetDir(assetDir)
    , m_assetName(assetDir.stem().string())
{
    m_stack.push_back(&m_root);
}

// ---------------------------------------------------------------------------
// Output accessors
// ---------------------------------------------------------------------------

std::string JsonAssetArchive::ToJsonString(int indent) const
{
    return m_root.dump(indent);
}

const nlohmann::json& JsonAssetArchive::GetRoot() const
{
    return m_root;
}

// ---------------------------------------------------------------------------
// Structural operations
// ---------------------------------------------------------------------------

void JsonAssetArchive::BeginObject(const std::string& className)
{
    auto& cur = *m_stack.back();
    if (cur.is_array())
    {
        cur.push_back(nlohmann::json::object());
        nlohmann::json& elem = cur.back();
        elem["_class"] = className;
        m_stack.push_back(&elem);
    }
    else
    {
        // Root-level object scope: annotate the current node and re-push it
        cur["_class"] = className;
        m_stack.push_back(&cur);
    }
}

std::string JsonAssetArchive::BeginObjectLoad()
{
    auto& cur = *m_stack.back();
    if (cur.is_array())
    {
        size_t idx = m_arrayIndex.back();
        m_arrayIndex.back()++;
        nlohmann::json& elem = cur[idx];
        m_stack.push_back(&elem);
        if (elem.contains("_class"))
            return elem["_class"].get<std::string>();
        return {};
    }
    else
    {
        // Root-level object
        m_stack.push_back(&cur);
        if (cur.contains("_class"))
            return cur["_class"].get<std::string>();
        return {};
    }
}

void JsonAssetArchive::EndObject()
{
    m_stack.pop_back();
}

void JsonAssetArchive::BeginArray(const std::string& key, size_t /*count*/)
{
    auto& cur = *m_stack.back();
    cur[key]  = nlohmann::json::array();
    m_stack.push_back(&cur[key]);
}

size_t JsonAssetArchive::BeginArrayLoad(const std::string& key)
{
    auto& cur = *m_stack.back();
    if (cur.contains(key) && cur[key].is_array())
    {
        m_stack.push_back(&cur[key]);
        m_arrayIndex.push_back(0);
        return cur[key].size();
    }
    m_stack.push_back(m_stack.back());
    m_arrayIndex.push_back(0);
    return 0;
}

void JsonAssetArchive::EndArray()
{
    m_stack.pop_back();
    if (IsLoading() && !m_arrayIndex.empty())
        m_arrayIndex.pop_back();
}

void JsonAssetArchive::BeginNestedArray(size_t /*count*/)
{
    auto& cur = *m_stack.back();
    cur.push_back(nlohmann::json::array());
    m_stack.push_back(&cur.back());
}

size_t JsonAssetArchive::BeginNestedArrayLoad()
{
    auto& cur = *m_stack.back();
    size_t idx = m_arrayIndex.back()++;
    nlohmann::json& elem = cur[idx];
    m_stack.push_back(&elem);
    m_arrayIndex.push_back(0);
    return elem.is_array() ? elem.size() : 0;
}

// ---------------------------------------------------------------------------
// Primitive serialization
// ---------------------------------------------------------------------------

void JsonAssetArchive::Serialize(const std::string& key, float& value)
{
    auto& cur = *m_stack.back();
    if (IsSaving())
        cur[key] = value;
    else if (cur.contains(key))
        try { value = cur[key].get<float>(); } catch (...) {}
}

void JsonAssetArchive::Serialize(const std::string& key, double& value)
{
    auto& cur = *m_stack.back();
    if (IsSaving())
        cur[key] = value;
    else if (cur.contains(key))
        try { value = cur[key].get<double>(); } catch (...) {}
}

void JsonAssetArchive::Serialize(const std::string& key, int& value)
{
    auto& cur = *m_stack.back();
    if (IsSaving())
        cur[key] = value;
    else if (cur.contains(key))
        try { value = cur[key].get<int>(); } catch (...) {}
}

void JsonAssetArchive::Serialize(const std::string& key, bool& value)
{
    auto& cur = *m_stack.back();
    if (IsSaving())
        cur[key] = value;
    else if (cur.contains(key))
        try { value = cur[key].get<bool>(); } catch (...) {}
}

void JsonAssetArchive::Serialize(const std::string& key, std::string& value)
{
    auto& cur = *m_stack.back();
    if (IsSaving())
        cur[key] = value;
    else if (cur.contains(key))
        try { value = cur[key].get<std::string>(); } catch (...) {}
}

void JsonAssetArchive::Serialize(const std::string& key, std::wstring& value)
{
    auto& cur = *m_stack.back();
    if (IsSaving())
    {
        cur[key] = WStringToUtf8(value);
    }
    else if (cur.contains(key))
    {
        try
        {
            value = Utf8ToWString(cur[key].get<std::string>());
        }
        catch (...) {}
    }
}

// ---------------------------------------------------------------------------
// Math type serialization
// ---------------------------------------------------------------------------

void JsonAssetArchive::Serialize(const std::string& key, DirectX::SimpleMath::Vector3& value)
{
    auto& cur = *m_stack.back();
    if (IsSaving())
    {
        cur[key] = nlohmann::json::array({ value.x, value.y, value.z });
    }
    else if (cur.contains(key))
    {
        try
        {
            auto& arr = cur[key];
            value.x   = arr[0].get<float>();
            value.y   = arr[1].get<float>();
            value.z   = arr[2].get<float>();
        }
        catch (...) {}
    }
}

void JsonAssetArchive::Serialize(const std::string& key, DirectX::SimpleMath::Quaternion& value)
{
    auto& cur = *m_stack.back();
    if (IsSaving())
    {
        cur[key] = nlohmann::json::array({ value.x, value.y, value.z, value.w });
    }
    else if (cur.contains(key))
    {
        try
        {
            auto& arr = cur[key];
            value.x   = arr[0].get<float>();
            value.y   = arr[1].get<float>();
            value.z   = arr[2].get<float>();
            value.w   = arr[3].get<float>();
        }
        catch (...) {}
    }
}

void JsonAssetArchive::Serialize(const std::string& key, DirectX::XMFLOAT4& value)
{
    auto& cur = *m_stack.back();
    if (IsSaving())
    {
        cur[key] = nlohmann::json::array({ value.x, value.y, value.z, value.w });
    }
    else if (cur.contains(key))
    {
        try
        {
            auto& arr = cur[key];
            value.x   = arr[0].get<float>();
            value.y   = arr[1].get<float>();
            value.z   = arr[2].get<float>();
            value.w   = arr[3].get<float>();
        }
        catch (...) {}
    }
}

void JsonAssetArchive::Serialize(const std::string& key, DirectX::XMFLOAT4X4& value)
{
    auto& cur = *m_stack.back();
    if (IsSaving())
    {
        nlohmann::json arr = nlohmann::json::array();
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                arr.push_back(value.m[r][c]);
        cur[key] = std::move(arr);
    }
    else if (cur.contains(key))
    {
        try
        {
            auto& arr = cur[key];
            for (int r = 0; r < 4; ++r)
                for (int c = 0; c < 4; ++c)
                    value.m[r][c] = arr[static_cast<size_t>(r * 4 + c)].get<float>();
        }
        catch (...) {}
    }
}

// ---------------------------------------------------------------------------
// Reference type serialization
// ---------------------------------------------------------------------------

void JsonAssetArchive::Serialize(const std::string& key, UUID& value)
{
    auto& cur = *m_stack.back();
    if (IsSaving())
    {
        cur[key] = value.ToString();
    }
    else if (cur.contains(key))
    {
        try { value = UUID::FromString(cur[key].get<std::string>()); } catch (...) {}
    }
}

void JsonAssetArchive::Serialize(const std::string& key, ScriptPointer& value)
{
    auto& cur = *m_stack.back();
    if (IsSaving())
    {
        cur[key] = nlohmann::json{
            { "assetId",  value.m_assetId.ToString()  },
            { "objectId", value.m_objectId.ToString() }
        };
    }
    else if (cur.contains(key))
    {
        try
        {
            auto& sp         = cur[key];
            value.m_assetId  = UUID::FromString(sp["assetId"].get<std::string>());
            value.m_objectId = UUID::FromString(sp["objectId"].get<std::string>());
        }
        catch (...) {}
    }
}

void JsonAssetArchive::Serialize(const std::string& key, BulkDataHandle& value)
{
    auto& cur = *m_stack.back();
    if (IsSaving())
    {
        cur[key] = nlohmann::json{
            { "_bulk", value.m_bulkId   },
            { "size",  value.m_dataSize }
        };
    }
    else if (cur.contains(key))
    {
        try
        {
            value.m_bulkId   = cur[key]["_bulk"].get<uint32_t>();
            value.m_dataSize = cur[key]["size"].get<uint64_t>();
        }
        catch (...) {}
    }
}

// ---------------------------------------------------------------------------
// Array element serialization
// ---------------------------------------------------------------------------

void JsonAssetArchive::SerializeElement(float& value)
{
    auto& cur = *m_stack.back();
    if (IsSaving())
        cur.push_back(value);
    else if (!m_arrayIndex.empty())
    {
        size_t idx = m_arrayIndex.back()++;
        if (idx < cur.size())
            try { value = cur[idx].get<float>(); } catch (...) {}
    }
}

void JsonAssetArchive::SerializeElement(double& value)
{
    auto& cur = *m_stack.back();
    if (IsSaving())
        cur.push_back(value);
    else if (!m_arrayIndex.empty())
    {
        size_t idx = m_arrayIndex.back()++;
        if (idx < cur.size())
            try { value = cur[idx].get<double>(); } catch (...) {}
    }
}

void JsonAssetArchive::SerializeElement(int& value)
{
    auto& cur = *m_stack.back();
    if (IsSaving())
        cur.push_back(value);
    else if (!m_arrayIndex.empty())
    {
        size_t idx = m_arrayIndex.back()++;
        if (idx < cur.size())
            try { value = cur[idx].get<int>(); } catch (...) {}
    }
}

void JsonAssetArchive::SerializeElement(bool& value)
{
    auto& cur = *m_stack.back();
    if (IsSaving())
        cur.push_back(value);
    else if (!m_arrayIndex.empty())
    {
        size_t idx = m_arrayIndex.back()++;
        if (idx < cur.size())
            try { value = cur[idx].get<bool>(); } catch (...) {}
    }
}

void JsonAssetArchive::SerializeElement(std::string& value)
{
    auto& cur = *m_stack.back();
    if (IsSaving())
        cur.push_back(value);
    else if (!m_arrayIndex.empty())
    {
        size_t idx = m_arrayIndex.back()++;
        if (idx < cur.size())
            try { value = cur[idx].get<std::string>(); } catch (...) {}
    }
}

void JsonAssetArchive::SerializeElement(std::wstring& value)
{
    auto& cur = *m_stack.back();
    if (IsSaving())
        cur.push_back(WStringToUtf8(value));
    else if (!m_arrayIndex.empty())
    {
        size_t idx = m_arrayIndex.back()++;
        if (idx < cur.size())
            try { value = Utf8ToWString(cur[idx].get<std::string>()); } catch (...) {}
    }
}

void JsonAssetArchive::SerializeElement(DirectX::SimpleMath::Vector3& value)
{
    auto& cur = *m_stack.back();
    if (IsSaving())
    {
        cur.push_back(nlohmann::json::array({ value.x, value.y, value.z }));
    }
    else if (!m_arrayIndex.empty())
    {
        size_t idx = m_arrayIndex.back()++;
        if (idx < cur.size())
        {
            try
            {
                auto& arr = cur[idx];
                value.x = arr[0].get<float>();
                value.y = arr[1].get<float>();
                value.z = arr[2].get<float>();
            }
            catch (...) {}
        }
    }
}

void JsonAssetArchive::SerializeElement(DirectX::SimpleMath::Quaternion& value)
{
    auto& cur = *m_stack.back();
    if (IsSaving())
    {
        cur.push_back(nlohmann::json::array({ value.x, value.y, value.z, value.w }));
    }
    else if (!m_arrayIndex.empty())
    {
        size_t idx = m_arrayIndex.back()++;
        if (idx < cur.size())
        {
            try
            {
                auto& arr = cur[idx];
                value.x = arr[0].get<float>();
                value.y = arr[1].get<float>();
                value.z = arr[2].get<float>();
                value.w = arr[3].get<float>();
            }
            catch (...) {}
        }
    }
}

void JsonAssetArchive::SerializeElement(DirectX::XMFLOAT4& value)
{
    auto& cur = *m_stack.back();
    if (IsSaving())
    {
        cur.push_back(nlohmann::json::array({ value.x, value.y, value.z, value.w }));
    }
    else if (!m_arrayIndex.empty())
    {
        size_t idx = m_arrayIndex.back()++;
        if (idx < cur.size())
        {
            try
            {
                auto& arr = cur[idx];
                value.x = arr[0].get<float>();
                value.y = arr[1].get<float>();
                value.z = arr[2].get<float>();
                value.w = arr[3].get<float>();
            }
            catch (...) {}
        }
    }
}

void JsonAssetArchive::SerializeElement(DirectX::XMFLOAT4X4& value)
{
    auto& cur = *m_stack.back();
    if (IsSaving())
    {
        nlohmann::json arr = nlohmann::json::array();
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                arr.push_back(value.m[r][c]);
        cur.push_back(std::move(arr));
    }
    else if (!m_arrayIndex.empty())
    {
        size_t idx = m_arrayIndex.back()++;
        if (idx < cur.size())
        {
            try
            {
                auto& arr = cur[idx];
                for (int r = 0; r < 4; ++r)
                    for (int c = 0; c < 4; ++c)
                        value.m[r][c] = arr[static_cast<size_t>(r * 4 + c)].get<float>();
            }
            catch (...) {}
        }
    }
}

void JsonAssetArchive::SerializeElement(ScriptPointer& value)
{
    auto& cur = *m_stack.back();
    if (IsSaving())
    {
        cur.push_back(nlohmann::json{
            { "assetId",  value.m_assetId.ToString()  },
            { "objectId", value.m_objectId.ToString() }
        });
    }
    else if (!m_arrayIndex.empty())
    {
        size_t idx = m_arrayIndex.back()++;
        if (idx < cur.size())
        {
            try
            {
                auto& sp         = cur[idx];
                value.m_assetId  = UUID::FromString(sp["assetId"].get<std::string>());
                value.m_objectId = UUID::FromString(sp["objectId"].get<std::string>());
            }
            catch (...) {}
        }
    }
}

// ---------------------------------------------------------------------------
// Bulk data I/O
// ---------------------------------------------------------------------------

void JsonAssetArchive::WriteBulkData(uint32_t bulkId, const void* data, uint64_t size)
{
    if (m_assetDir.empty())
        return;

    const std::string filename = m_assetName + "_Bulk" + std::to_string(bulkId) + ".bin";
    const std::filesystem::path filePath = m_assetDir / filename;

    std::ofstream ofs(filePath, std::ios::binary | std::ios::trunc);
    if (ofs)
        ofs.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));

    m_root["header"]["bulkDataMap"][std::to_string(bulkId)] = {
        { "file", filename },
        { "size", size     }
    };
}

std::vector<uint8_t> JsonAssetArchive::ReadBulkData(uint32_t bulkId)
{
    try
    {
        const auto& map      = m_root["header"]["bulkDataMap"];
        const std::string key = std::to_string(bulkId);
        if (!map.contains(key))
            return {};

        const std::string filename = map[key]["file"].get<std::string>();
        const std::filesystem::path filePath = m_assetDir / filename;

        std::ifstream ifs(filePath, std::ios::binary | std::ios::ate);
        if (!ifs)
            return {};

        const auto fileSize = static_cast<size_t>(ifs.tellg());
        ifs.seekg(0);

        std::vector<uint8_t> buf(fileSize);
        ifs.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(fileSize));
        return buf;
    }
    catch (...)
    {
        return {};
    }
}
