#include "Serialization/JsonAssetArchive.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <fstream>

using namespace DeltaEngine;

namespace DeltaEngine
{
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

template <typename SaveFn, typename LoadFn>
void StoreOrLoadNode(nlohmann::json& cur, bool isSaving, const std::string& key, SaveFn&& saveFn, LoadFn&& loadFn)
{
    if (isSaving)
    {
        cur[key] = saveFn();
        return;
    }

    if (!cur.contains(key))
        return;

    try
    {
        loadFn(cur[key]);
    }
    catch (...)
    {
    }
}

template <typename SaveFn, typename LoadFn>
void StoreOrLoadElementNode(
    nlohmann::json& cur,
    bool isSaving,
    std::vector<size_t>& arrayIndex,
    SaveFn&& saveFn,
    LoadFn&& loadFn)
{
    if (isSaving)
    {
        cur.push_back(saveFn());
        return;
    }

    if (arrayIndex.empty())
        return;

    const size_t idx = arrayIndex.back()++;
    if (idx >= cur.size())
        return;

    try
    {
        loadFn(cur[idx]);
    }
    catch (...)
    {
    }
}

template <typename T>
void StoreOrLoadValue(nlohmann::json& cur, bool isSaving, const std::string& key, T& value)
{
    StoreOrLoadNode(
        cur,
        isSaving,
        key,
        [&]() -> nlohmann::json { return value; },
        [&](const nlohmann::json& node) { value = node.get<T>(); });
}

template <typename T>
void StoreOrLoadElementValue(nlohmann::json& cur, bool isSaving, std::vector<size_t>& arrayIndex, T& value)
{
    StoreOrLoadElementNode(
        cur,
        isSaving,
        arrayIndex,
        [&]() -> nlohmann::json { return value; },
        [&](const nlohmann::json& node) { value = node.get<T>(); });
}

}

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

std::string JsonAssetArchive::ToJsonString(int indent) const
{
    return m_root.dump(indent);
}

const nlohmann::json& JsonAssetArchive::GetRoot() const
{
    return m_root;
}

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

void JsonAssetArchive::BeginNestedObject(const std::string& key)
{
    auto& cur = *m_stack.back();
    cur[key]  = nlohmann::json::object();
    m_stack.push_back(&cur[key]);
}

bool JsonAssetArchive::BeginNestedObjectLoad(const std::string& key)
{
    auto& cur = *m_stack.back();
    if (!cur.contains(key) || !cur[key].is_object())
        return false;
    m_stack.push_back(&cur[key]);
    return true;
}

void JsonAssetArchive::EndNestedObject()
{
    if (m_stack.size() > 1)
        m_stack.pop_back();
}

void JsonAssetArchive::Serialize(const std::string& key, float& value)
{
    auto& cur = *m_stack.back();
    StoreOrLoadValue(cur, IsSaving(), key, value);
}

void JsonAssetArchive::Serialize(const std::string& key, double& value)
{
    auto& cur = *m_stack.back();
    StoreOrLoadValue(cur, IsSaving(), key, value);
}

void JsonAssetArchive::Serialize(const std::string& key, int& value)
{
    auto& cur = *m_stack.back();
    StoreOrLoadValue(cur, IsSaving(), key, value);
}

void JsonAssetArchive::Serialize(const std::string& key, bool& value)
{
    auto& cur = *m_stack.back();
    StoreOrLoadValue(cur, IsSaving(), key, value);
}

void JsonAssetArchive::Serialize(const std::string& key, std::string& value)
{
    auto& cur = *m_stack.back();
    StoreOrLoadValue(cur, IsSaving(), key, value);
}

void JsonAssetArchive::Serialize(const std::string& key, std::wstring& value)
{
    auto& cur = *m_stack.back();
    StoreOrLoadNode(
        cur,
        IsSaving(),
        key,
        [&]() -> nlohmann::json { return WStringToUtf8(value); },
        [&](const nlohmann::json& node) { value = Utf8ToWString(node.get<std::string>()); });
}

void JsonAssetArchive::Serialize(const std::string& key, DirectX::SimpleMath::Vector3& value)
{
    auto& cur = *m_stack.back();
    StoreOrLoadNode(
        cur,
        IsSaving(),
        key,
        [&]() -> nlohmann::json { return nlohmann::json::array({ value.x, value.y, value.z }); },
        [&](const nlohmann::json& node)
        {
            value.x = node[0].get<float>();
            value.y = node[1].get<float>();
            value.z = node[2].get<float>();
        });
}

void JsonAssetArchive::Serialize(const std::string& key, DirectX::SimpleMath::Quaternion& value)
{
    auto& cur = *m_stack.back();
    StoreOrLoadNode(
        cur,
        IsSaving(),
        key,
        [&]() -> nlohmann::json { return nlohmann::json::array({ value.x, value.y, value.z, value.w }); },
        [&](const nlohmann::json& node)
        {
            value.x = node[0].get<float>();
            value.y = node[1].get<float>();
            value.z = node[2].get<float>();
            value.w = node[3].get<float>();
        });
}

void JsonAssetArchive::Serialize(const std::string& key, DirectX::XMFLOAT4& value)
{
    auto& cur = *m_stack.back();
    StoreOrLoadNode(
        cur,
        IsSaving(),
        key,
        [&]() -> nlohmann::json { return nlohmann::json::array({ value.x, value.y, value.z, value.w }); },
        [&](const nlohmann::json& node)
        {
            value.x = node[0].get<float>();
            value.y = node[1].get<float>();
            value.z = node[2].get<float>();
            value.w = node[3].get<float>();
        });
}

void JsonAssetArchive::Serialize(const std::string& key, DirectX::XMFLOAT4X4& value)
{
    auto& cur = *m_stack.back();
    StoreOrLoadNode(
        cur,
        IsSaving(),
        key,
        [&]() -> nlohmann::json
        {
            nlohmann::json arr = nlohmann::json::array();
            for (int r = 0; r < 4; ++r)
                for (int c = 0; c < 4; ++c)
                    arr.push_back(value.m[r][c]);
            return arr;
        },
        [&](const nlohmann::json& node)
        {
            for (int r = 0; r < 4; ++r)
                for (int c = 0; c < 4; ++c)
                    value.m[r][c] = node[static_cast<size_t>(r * 4 + c)].get<float>();
        });
}

void JsonAssetArchive::Serialize(const std::string& key, UUID& value)
{
    auto& cur = *m_stack.back();
    StoreOrLoadNode(
        cur,
        IsSaving(),
        key,
        [&]() -> nlohmann::json { return value.ToString(); },
        [&](const nlohmann::json& node) { value = UUID::FromString(node.get<std::string>()); });
}

void JsonAssetArchive::Serialize(const std::string& key, ScriptPointer& value)
{
    auto& cur = *m_stack.back();
    StoreOrLoadNode(
        cur,
        IsSaving(),
        key,
        [&]() -> nlohmann::json
        {
            return nlohmann::json{
                { "assetId",  value.m_assetId.ToString()  },
                { "objectId", value.m_objectId.ToString() }
            };
        },
        [&](const nlohmann::json& node)
        {
            value.m_assetId  = UUID::FromString(node["assetId"].get<std::string>());
            value.m_objectId = UUID::FromString(node["objectId"].get<std::string>());
        });
}

void JsonAssetArchive::Serialize(const std::string& key, BulkDataHandle& value)
{
    auto& cur = *m_stack.back();
    StoreOrLoadNode(
        cur,
        IsSaving(),
        key,
        [&]() -> nlohmann::json
        {
            return nlohmann::json{
                { "_bulk", value.m_bulkId   },
                { "size",  value.m_dataSize }
            };
        },
        [&](const nlohmann::json& node)
        {
            value.m_bulkId   = node["_bulk"].get<uint32_t>();
            value.m_dataSize = node["size"].get<uint64_t>();
        });
}

void JsonAssetArchive::SerializeElement(float& value)
{
    auto& cur = *m_stack.back();
    StoreOrLoadElementValue(cur, IsSaving(), m_arrayIndex, value);
}

void JsonAssetArchive::SerializeElement(double& value)
{
    auto& cur = *m_stack.back();
    StoreOrLoadElementValue(cur, IsSaving(), m_arrayIndex, value);
}

void JsonAssetArchive::SerializeElement(int& value)
{
    auto& cur = *m_stack.back();
    StoreOrLoadElementValue(cur, IsSaving(), m_arrayIndex, value);
}

void JsonAssetArchive::SerializeElement(bool& value)
{
    auto& cur = *m_stack.back();
    StoreOrLoadElementValue(cur, IsSaving(), m_arrayIndex, value);
}

void JsonAssetArchive::SerializeElement(std::string& value)
{
    auto& cur = *m_stack.back();
    StoreOrLoadElementValue(cur, IsSaving(), m_arrayIndex, value);
}

void JsonAssetArchive::SerializeElement(std::wstring& value)
{
    auto& cur = *m_stack.back();
    StoreOrLoadElementNode(
        cur,
        IsSaving(),
        m_arrayIndex,
        [&]() -> nlohmann::json { return WStringToUtf8(value); },
        [&](const nlohmann::json& node) { value = Utf8ToWString(node.get<std::string>()); });
}

void JsonAssetArchive::SerializeElement(DirectX::SimpleMath::Vector3& value)
{
    auto& cur = *m_stack.back();
    StoreOrLoadElementNode(
        cur,
        IsSaving(),
        m_arrayIndex,
        [&]() -> nlohmann::json { return nlohmann::json::array({ value.x, value.y, value.z }); },
        [&](const nlohmann::json& node)
        {
            value.x = node[0].get<float>();
            value.y = node[1].get<float>();
            value.z = node[2].get<float>();
        });
}

void JsonAssetArchive::SerializeElement(DirectX::SimpleMath::Quaternion& value)
{
    auto& cur = *m_stack.back();
    StoreOrLoadElementNode(
        cur,
        IsSaving(),
        m_arrayIndex,
        [&]() -> nlohmann::json { return nlohmann::json::array({ value.x, value.y, value.z, value.w }); },
        [&](const nlohmann::json& node)
        {
            value.x = node[0].get<float>();
            value.y = node[1].get<float>();
            value.z = node[2].get<float>();
            value.w = node[3].get<float>();
        });
}

void JsonAssetArchive::SerializeElement(DirectX::XMFLOAT4& value)
{
    auto& cur = *m_stack.back();
    StoreOrLoadElementNode(
        cur,
        IsSaving(),
        m_arrayIndex,
        [&]() -> nlohmann::json { return nlohmann::json::array({ value.x, value.y, value.z, value.w }); },
        [&](const nlohmann::json& node)
        {
            value.x = node[0].get<float>();
            value.y = node[1].get<float>();
            value.z = node[2].get<float>();
            value.w = node[3].get<float>();
        });
}

void JsonAssetArchive::SerializeElement(DirectX::XMFLOAT4X4& value)
{
    auto& cur = *m_stack.back();
    StoreOrLoadElementNode(
        cur,
        IsSaving(),
        m_arrayIndex,
        [&]() -> nlohmann::json
        {
            nlohmann::json arr = nlohmann::json::array();
            for (int r = 0; r < 4; ++r)
                for (int c = 0; c < 4; ++c)
                    arr.push_back(value.m[r][c]);
            return arr;
        },
        [&](const nlohmann::json& node)
        {
            for (int r = 0; r < 4; ++r)
                for (int c = 0; c < 4; ++c)
                    value.m[r][c] = node[static_cast<size_t>(r * 4 + c)].get<float>();
        });
}

void JsonAssetArchive::SerializeElement(ScriptPointer& value)
{
    auto& cur = *m_stack.back();
    StoreOrLoadElementNode(
        cur,
        IsSaving(),
        m_arrayIndex,
        [&]() -> nlohmann::json
        {
            return nlohmann::json{
                { "assetId",  value.m_assetId.ToString()  },
                { "objectId", value.m_objectId.ToString() }
            };
        },
        [&](const nlohmann::json& node)
        {
            value.m_assetId  = UUID::FromString(node["assetId"].get<std::string>());
            value.m_objectId = UUID::FromString(node["objectId"].get<std::string>());
        });
}

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
