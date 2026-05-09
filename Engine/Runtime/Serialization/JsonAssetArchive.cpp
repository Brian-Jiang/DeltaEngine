#include "Runtime/Serialization/JsonAssetArchive.h"

#include <fstream>
#include <system_error>

using namespace DeltaEngine;

namespace DeltaEngine
{
static nlohmann::json& CurrentNode(std::vector<nlohmann::json*>& stack, const char* operationName)
{
    DELTA_VERIFY_MSG(!stack.empty(), "JsonAssetArchive {} called with an empty stack", operationName);
    DELTA_VERIFY_MSG(stack.back() != nullptr, "JsonAssetArchive {} called with a null stack entry", operationName);
    return *stack.back();
}

static bool IsSafeRelativeBulkPath(const std::filesystem::path& relativePath)
{
    if (relativePath.empty() || relativePath.is_absolute() ||
        relativePath.has_root_name() || relativePath.has_root_directory())
        return false;

    for (const std::filesystem::path& part : relativePath)
    {
        if (part == "..")
            return false;
    }

    return true;
}

static bool IsSafeBulkAssetName(const std::string& assetName)
{
    const std::filesystem::path assetNamePath(assetName);
    return !assetName.empty() &&
           assetNamePath.filename() == assetNamePath &&
           IsSafeRelativeBulkPath(assetNamePath);
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
    catch (const nlohmann::json::exception& ex)
    {
        DLOG(LogSerialization, ELogLevel::Warning,
             "Failed to load JSON field '{}': malformed value '{}' (expected compatible serialized type; error: {})",
             key, cur[key].dump(), ex.what());
        DELTA_ENSURE_MSG(false, "JsonAssetArchive malformed field");
    }
    catch (const std::exception& ex)
    {
        DLOG(LogSerialization, ELogLevel::Warning,
             "Failed to load JSON field '{}': unexpected error '{}' (expected compatible serialized type)",
             key, ex.what());
        DELTA_ENSURE_MSG(false, "JsonAssetArchive field load failed");
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
    {
        DLOG(LogSerialization, ELogLevel::Warning,
             "Failed to load JSON array element: no active array index (expected BeginArrayLoad or BeginNestedArrayLoad)");
        DELTA_ENSURE_MSG(false, "JsonAssetArchive array element loaded outside array scope");
        return;
    }

    const size_t idx = arrayIndex.back()++;
    if (idx >= cur.size())
    {
        DLOG(LogSerialization, ELogLevel::Warning,
             "Failed to load JSON array element {}: index out of range (array size: {})",
             idx, cur.size());
        DELTA_ENSURE_MSG(false, "JsonAssetArchive array element index out of range");
        return;
    }

    try
    {
        loadFn(cur[idx]);
    }
    catch (const nlohmann::json::exception& ex)
    {
        DLOG(LogSerialization, ELogLevel::Warning,
             "Failed to load JSON array element {}: malformed value '{}' (expected compatible serialized type; error: {})",
             idx, cur[idx].dump(), ex.what());
        DELTA_ENSURE_MSG(false, "JsonAssetArchive malformed array element");
    }
    catch (const std::exception& ex)
    {
        DLOG(LogSerialization, ELogLevel::Warning,
             "Failed to load JSON array element {}: unexpected error '{}' (expected compatible serialized type)",
             idx, ex.what());
        DELTA_ENSURE_MSG(false, "JsonAssetArchive array element load failed");
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
    auto& cur = CurrentNode(m_stack, "BeginObject");
    if (cur.is_array())
    {
        cur.push_back(nlohmann::json::object());
        nlohmann::json& elem = cur.back();
        elem["_class"] = className;
        m_stack.push_back(&elem);
    }
    else if (cur.is_object())
    {
        cur["_class"] = className;
        m_stack.push_back(&cur);
    }
    else
    {
        DLOG(LogSerialization, ELogLevel::Error,
             "Failed to begin object '{}': current JSON node is '{}' (expected object or array)",
             className, cur.type_name());
        DELTA_VERIFY_MSG(false, "JsonAssetArchive BeginObject requires object or array node");
    }
}

std::string JsonAssetArchive::BeginObjectLoad()
{
    auto& cur = CurrentNode(m_stack, "BeginObjectLoad");
    if (cur.is_array())
    {
        if (!DELTA_ENSURE_MSG(!m_arrayIndex.empty(), "JsonAssetArchive BeginObjectLoad array scope missing index"))
        {
            DLOG(LogSerialization, ELogLevel::Warning,
                 "Failed to begin object load from array: no active array index (expected BeginArrayLoad)");
            m_stack.push_back(&cur);
            return {};
        }
        size_t idx = m_arrayIndex.back();
        m_arrayIndex.back()++;
        if (idx >= cur.size())
        {
            DLOG(LogSerialization, ELogLevel::Warning,
                 "Failed to begin object load from array: index {} out of range (array size: {})",
                 idx, cur.size());
            m_stack.push_back(&cur);
            return {};
        }
        nlohmann::json& elem = cur[idx];
        m_stack.push_back(&elem);
        if (elem.contains("_class"))
        {
            try
            {
                return elem["_class"].get<std::string>();
            }
            catch (const nlohmann::json::exception& ex)
            {
                DLOG(LogSerialization, ELogLevel::Warning,
                     "Failed to load object class name at array index {}: malformed '_class' value '{}' (expected string; error: {})",
                     idx, elem["_class"].dump(), ex.what());
                DELTA_ENSURE_MSG(false, "JsonAssetArchive malformed object class");
            }
        }
        DLOG(LogSerialization, ELogLevel::Warning,
             "Failed to load object class name at array index {}: missing '_class' field (expected reflected class name)",
             idx);
        return {};
    }
    else
    {
        m_stack.push_back(&cur);
        if (cur.contains("_class"))
        {
            try
            {
                return cur["_class"].get<std::string>();
            }
            catch (const nlohmann::json::exception& ex)
            {
                DLOG(LogSerialization, ELogLevel::Warning,
                     "Failed to load object class name: malformed '_class' value '{}' (expected string; error: {})",
                     cur["_class"].dump(), ex.what());
                DELTA_ENSURE_MSG(false, "JsonAssetArchive malformed object class");
            }
        }
        DLOG(LogSerialization, ELogLevel::Warning,
             "Failed to load object class name: missing '_class' field (expected reflected class name)");
        return {};
    }
}

void JsonAssetArchive::EndObject()
{
    DELTA_VERIFY_MSG(m_stack.size() > 1, "JsonAssetArchive EndObject called without a matching BeginObject");
    m_stack.pop_back();
}

void JsonAssetArchive::BeginArray(const std::string& key, size_t /*count*/)
{
    auto& cur = CurrentNode(m_stack, "BeginArray");
    DELTA_VERIFY_MSG(cur.is_object(), "JsonAssetArchive BeginArray requires an object node");
    cur[key]  = nlohmann::json::array();
    m_stack.push_back(&cur[key]);
}

size_t JsonAssetArchive::BeginArrayLoad(const std::string& key)
{
    auto& cur = CurrentNode(m_stack, "BeginArrayLoad");
    if (cur.contains(key) && cur[key].is_array())
    {
        m_stack.push_back(&cur[key]);
        m_arrayIndex.push_back(0);
        return cur[key].size();
    }
    if (cur.contains(key))
    {
        DLOG(LogSerialization, ELogLevel::Warning,
             "Failed to begin array load for '{}': JSON node is '{}' (expected array)",
             key, cur[key].type_name());
        DELTA_ENSURE_MSG(false, "JsonAssetArchive array field has wrong type");
    }
    m_stack.push_back(m_stack.back());
    m_arrayIndex.push_back(0);
    return 0;
}

void JsonAssetArchive::EndArray()
{
    DELTA_VERIFY_MSG(m_stack.size() > 1, "JsonAssetArchive EndArray called without a matching BeginArray");
    m_stack.pop_back();
    if (IsLoading() && !m_arrayIndex.empty())
        m_arrayIndex.pop_back();
}

void JsonAssetArchive::BeginNestedArray(size_t /*count*/)
{
    auto& cur = CurrentNode(m_stack, "BeginNestedArray");
    DELTA_VERIFY_MSG(cur.is_array(), "JsonAssetArchive BeginNestedArray requires an array node");
    cur.push_back(nlohmann::json::array());
    m_stack.push_back(&cur.back());
}

size_t JsonAssetArchive::BeginNestedArrayLoad()
{
    auto& cur = CurrentNode(m_stack, "BeginNestedArrayLoad");
    if (!DELTA_ENSURE_MSG(cur.is_array(), "JsonAssetArchive BeginNestedArrayLoad requires an array node"))
    {
        DLOG(LogSerialization, ELogLevel::Warning,
             "Failed to begin nested array load: current JSON node is '{}' (expected array)",
             cur.type_name());
        m_stack.push_back(&cur);
        m_arrayIndex.push_back(0);
        return 0;
    }
    if (!DELTA_ENSURE_MSG(!m_arrayIndex.empty(), "JsonAssetArchive BeginNestedArrayLoad missing array index"))
    {
        DLOG(LogSerialization, ELogLevel::Warning,
             "Failed to begin nested array load: no active array index (expected parent array scope)");
        m_stack.push_back(&cur);
        m_arrayIndex.push_back(0);
        return 0;
    }
    size_t idx = m_arrayIndex.back()++;
    if (idx >= cur.size())
    {
        DLOG(LogSerialization, ELogLevel::Warning,
             "Failed to begin nested array load: index {} out of range (array size: {})",
             idx, cur.size());
        m_stack.push_back(&cur);
        m_arrayIndex.push_back(0);
        return 0;
    }
    nlohmann::json& elem = cur[idx];
    m_stack.push_back(&elem);
    m_arrayIndex.push_back(0);
    if (!elem.is_array())
    {
        DLOG(LogSerialization, ELogLevel::Warning,
             "Failed to begin nested array load at index {}: JSON node is '{}' (expected array)",
             idx, elem.type_name());
        DELTA_ENSURE_MSG(false, "JsonAssetArchive nested array element has wrong type");
        return 0;
    }
    return elem.is_array() ? elem.size() : 0;
}

void JsonAssetArchive::BeginNestedObject(const std::string& key)
{
    auto& cur = CurrentNode(m_stack, "BeginNestedObject");
    DELTA_VERIFY_MSG(cur.is_object(), "JsonAssetArchive BeginNestedObject requires an object node");
    cur[key]  = nlohmann::json::object();
    m_stack.push_back(&cur[key]);
}

bool JsonAssetArchive::BeginNestedObjectLoad(const std::string& key)
{
    auto& cur = CurrentNode(m_stack, "BeginNestedObjectLoad");
    if (!cur.contains(key) || !cur[key].is_object())
    {
        if (cur.contains(key))
        {
            DLOG(LogSerialization, ELogLevel::Warning,
                 "Failed to begin nested object load for '{}': JSON node is '{}' (expected object)",
                 key, cur[key].type_name());
            DELTA_ENSURE_MSG(false, "JsonAssetArchive nested object field has wrong type");
        }
        return false;
    }
    m_stack.push_back(&cur[key]);
    return true;
}

void JsonAssetArchive::EndNestedObject()
{
    DELTA_VERIFY_MSG(m_stack.size() > 1, "JsonAssetArchive EndNestedObject called without a matching BeginNestedObject");
    m_stack.pop_back();
}

void JsonAssetArchive::Serialize(const std::string& key, float& value)
{
    auto& cur = CurrentNode(m_stack, "SerializeFloat");
    StoreOrLoadValue(cur, IsSaving(), key, value);
}

void JsonAssetArchive::Serialize(const std::string& key, double& value)
{
    auto& cur = CurrentNode(m_stack, "SerializeDouble");
    StoreOrLoadValue(cur, IsSaving(), key, value);
}

void JsonAssetArchive::Serialize(const std::string& key, int& value)
{
    auto& cur = CurrentNode(m_stack, "SerializeInt");
    StoreOrLoadValue(cur, IsSaving(), key, value);
}

void JsonAssetArchive::Serialize(const std::string& key, bool& value)
{
    auto& cur = CurrentNode(m_stack, "SerializeBool");
    StoreOrLoadValue(cur, IsSaving(), key, value);
}

void JsonAssetArchive::Serialize(const std::string& key, std::string& value)
{
    auto& cur = CurrentNode(m_stack, "SerializeString");
    StoreOrLoadValue(cur, IsSaving(), key, value);
}

void JsonAssetArchive::Serialize(const std::string& key, DirectX::SimpleMath::Vector3& value)
{
    auto& cur = CurrentNode(m_stack, "SerializeVector3");
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
    auto& cur = CurrentNode(m_stack, "SerializeQuaternion");
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
    auto& cur = CurrentNode(m_stack, "SerializeFloat4");
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
    auto& cur = CurrentNode(m_stack, "SerializeFloat4x4");
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

void JsonAssetArchive::Serialize(const std::string& key, DirectX::BoundingBox& value)
{
    auto& cur = CurrentNode(m_stack, "SerializeBoundingBox");
    StoreOrLoadNode(
        cur,
        IsSaving(),
        key,
        [&]() -> nlohmann::json
        {
            return nlohmann::json::array({
                value.Center.x,  value.Center.y,  value.Center.z,
                value.Extents.x, value.Extents.y, value.Extents.z });
        },
        [&](const nlohmann::json& node)
        {
            value.Center.x  = node[0].get<float>();
            value.Center.y  = node[1].get<float>();
            value.Center.z  = node[2].get<float>();
            value.Extents.x = node[3].get<float>();
            value.Extents.y = node[4].get<float>();
            value.Extents.z = node[5].get<float>();
        });
}

void JsonAssetArchive::Serialize(const std::string& key, UUID& value)
{
    auto& cur = CurrentNode(m_stack, "SerializeUUID");
    StoreOrLoadNode(
        cur,
        IsSaving(),
        key,
        [&]() -> nlohmann::json { return value.ToString(); },
        [&](const nlohmann::json& node) { value = UUID::FromString(node.get<std::string>()); });
}

void JsonAssetArchive::Serialize(const std::string& key, ScriptPointer& value)
{
    auto& cur = CurrentNode(m_stack, "SerializeScriptPointer");
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
    auto& cur = CurrentNode(m_stack, "SerializeBulkDataHandle");
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
    auto& cur = CurrentNode(m_stack, "SerializeElementFloat");
    StoreOrLoadElementValue(cur, IsSaving(), m_arrayIndex, value);
}

void JsonAssetArchive::SerializeElement(double& value)
{
    auto& cur = CurrentNode(m_stack, "SerializeElementDouble");
    StoreOrLoadElementValue(cur, IsSaving(), m_arrayIndex, value);
}

void JsonAssetArchive::SerializeElement(int& value)
{
    auto& cur = CurrentNode(m_stack, "SerializeElementInt");
    StoreOrLoadElementValue(cur, IsSaving(), m_arrayIndex, value);
}

void JsonAssetArchive::SerializeElement(bool& value)
{
    auto& cur = CurrentNode(m_stack, "SerializeElementBool");
    StoreOrLoadElementValue(cur, IsSaving(), m_arrayIndex, value);
}

void JsonAssetArchive::SerializeElement(std::string& value)
{
    auto& cur = CurrentNode(m_stack, "SerializeElementString");
    StoreOrLoadElementValue(cur, IsSaving(), m_arrayIndex, value);
}

void JsonAssetArchive::SerializeElement(DirectX::SimpleMath::Vector3& value)
{
    auto& cur = CurrentNode(m_stack, "SerializeElementVector3");
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
    auto& cur = CurrentNode(m_stack, "SerializeElementQuaternion");
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
    auto& cur = CurrentNode(m_stack, "SerializeElementFloat4");
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
    auto& cur = CurrentNode(m_stack, "SerializeElementFloat4x4");
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

void JsonAssetArchive::SerializeElement(DirectX::BoundingBox& value)
{
    auto& cur = CurrentNode(m_stack, "SerializeElementBoundingBox");
    StoreOrLoadElementNode(
        cur,
        IsSaving(),
        m_arrayIndex,
        [&]() -> nlohmann::json
        {
            return nlohmann::json::array({
                value.Center.x,  value.Center.y,  value.Center.z,
                value.Extents.x, value.Extents.y, value.Extents.z });
        },
        [&](const nlohmann::json& node)
        {
            value.Center.x  = node[0].get<float>();
            value.Center.y  = node[1].get<float>();
            value.Center.z  = node[2].get<float>();
            value.Extents.x = node[3].get<float>();
            value.Extents.y = node[4].get<float>();
            value.Extents.z = node[5].get<float>();
        });
}

void JsonAssetArchive::SerializeElement(ScriptPointer& value)
{
    auto& cur = CurrentNode(m_stack, "SerializeElementScriptPointer");
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
    {
        DLOG(LogSerialization, ELogLevel::Warning,
             "Failed to write bulk data {}: asset directory is empty (expected writable asset directory)",
             bulkId);
        DELTA_ENSURE_MSG(false, "JsonAssetArchive WriteBulkData requires asset directory");
        return;
    }

    if (size > 0 && !data)
    {
        DLOG(LogSerialization, ELogLevel::Error,
             "Failed to write bulk data {}: payload pointer is null but size is {} bytes (expected non-null data)",
             bulkId, size);
        DELTA_ENSURE_MSG(false, "JsonAssetArchive WriteBulkData received null data");
        return;
    }

    if (!IsSafeBulkAssetName(m_assetName))
    {
        DLOG(LogSerialization, ELogLevel::Error,
             "Failed to write bulk data {}: asset name '{}' is unsafe (expected filename stem without path separators)",
             bulkId, m_assetName);
        DELTA_ENSURE_MSG(false, "JsonAssetArchive WriteBulkData rejected unsafe asset name");
        return;
    }

    std::error_code errorCode;
    std::filesystem::create_directories(m_assetDir, errorCode);
    if (errorCode || !std::filesystem::is_directory(m_assetDir, errorCode))
    {
        DLOG(LogSerialization, ELogLevel::Error,
             "Failed to write bulk data {}: asset directory '{}' is not writable (expected directory; error: {})",
             bulkId, m_assetDir.string(), errorCode.message());
        DELTA_ENSURE_MSG(false, "JsonAssetArchive WriteBulkData requires writable directory");
        return;
    }

    const std::string filename = m_assetName + "_Bulk" + std::to_string(bulkId) + ".bin";
    const std::filesystem::path filePath = m_assetDir / filename;

    std::ofstream ofs(filePath, std::ios::binary | std::ios::trunc);
    if (!ofs)
    {
        DLOG(LogSerialization, ELogLevel::Error,
             "Failed to write bulk data {}: could not open '{}' (expected writable binary sidecar)",
             bulkId, filePath.string());
        DELTA_ENSURE_MSG(false, "JsonAssetArchive WriteBulkData failed to open file");
        return;
    }

    if (size > 0)
        ofs.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
    ofs.close();
    if (!ofs)
    {
        DLOG(LogSerialization, ELogLevel::Error,
             "Failed to write bulk data {}: short write for '{}' (expected {} bytes)",
             bulkId, filePath.string(), size);
        std::filesystem::remove(filePath, errorCode);
        DELTA_ENSURE_MSG(false, "JsonAssetArchive WriteBulkData failed during write");
        return;
    }

    m_root["header"]["bulkDataMap"][std::to_string(bulkId)] = {
        { "file", filename },
        { "size", size     }
    };
}

std::vector<uint8_t> JsonAssetArchive::ReadBulkData(uint32_t bulkId)
{
    try
    {
        if (m_assetDir.empty())
        {
            DLOG(LogSerialization, ELogLevel::Warning,
                 "Failed to read bulk data {}: asset directory is empty (expected asset directory containing sidecars)",
                 bulkId);
            DELTA_ENSURE_MSG(false, "JsonAssetArchive ReadBulkData requires asset directory");
            return {};
        }

        if (!m_root.contains("header") || !m_root["header"].contains("bulkDataMap") ||
            !m_root["header"]["bulkDataMap"].is_object())
        {
            DLOG(LogSerialization, ELogLevel::Warning,
                 "Failed to read bulk data {}: bulkDataMap is missing or malformed (expected header.bulkDataMap object)",
                 bulkId);
            return {};
        }

        const auto& map      = m_root["header"]["bulkDataMap"];
        const std::string key = std::to_string(bulkId);
        if (!map.contains(key))
        {
            DLOG(LogSerialization, ELogLevel::Warning,
                 "Failed to read bulk data {}: map entry is missing (expected key '{}')",
                 bulkId, key);
            return {};
        }

        const auto& entry = map[key];
        if (!entry.is_object() || !entry.contains("file") || !entry["file"].is_string() ||
            !entry.contains("size") || !entry["size"].is_number_integer())
        {
            DLOG(LogSerialization, ELogLevel::Warning,
                 "Failed to read bulk data {}: map entry '{}' is malformed (expected file string and size integer)",
                 bulkId, key);
            DELTA_ENSURE_MSG(false, "JsonAssetArchive malformed bulkDataMap entry");
            return {};
        }

        const std::string filename = entry["file"].get<std::string>();
        const std::filesystem::path relativePath(filename);
        if (!IsSafeRelativeBulkPath(relativePath))
        {
            DLOG(LogSerialization, ELogLevel::Error,
                 "Failed to read bulk data {}: sidecar path '{}' is unsafe (expected relative path inside asset directory)",
                 bulkId, filename);
            DELTA_ENSURE_MSG(false, "JsonAssetArchive rejected unsafe bulk sidecar path");
            return {};
        }

        const int64_t expectedSizeValue = entry["size"].get<int64_t>();
        if (expectedSizeValue < 0)
        {
            DLOG(LogSerialization, ELogLevel::Warning,
                 "Failed to read bulk data {}: map entry size is {} (expected non-negative byte count)",
                 bulkId, expectedSizeValue);
            DELTA_ENSURE_MSG(false, "JsonAssetArchive negative bulk size");
            return {};
        }
        const uint64_t expectedSize = static_cast<uint64_t>(expectedSizeValue);
        const std::filesystem::path filePath = m_assetDir / relativePath;

        std::ifstream ifs(filePath, std::ios::binary | std::ios::ate);
        if (!ifs)
        {
            DLOG(LogSerialization, ELogLevel::Error,
                 "Failed to read bulk data {}: sidecar '{}' could not be opened (expected readable file)",
                 bulkId, filePath.string());
            DELTA_ENSURE_MSG(false, "JsonAssetArchive ReadBulkData failed to open sidecar");
            return {};
        }

        const auto fileSize = static_cast<size_t>(ifs.tellg());
        if (fileSize != expectedSize)
        {
            DLOG(LogSerialization, ELogLevel::Error,
                 "Failed to read bulk data {}: sidecar '{}' has {} bytes (expected {} bytes)",
                 bulkId, filePath.string(), fileSize, expectedSize);
            DELTA_ENSURE_MSG(false, "JsonAssetArchive ReadBulkData size mismatch");
            return {};
        }
        ifs.seekg(0);

        std::vector<uint8_t> buf(fileSize);
        if (fileSize > 0)
            ifs.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(fileSize));
        if (!ifs && fileSize > 0)
        {
            DLOG(LogSerialization, ELogLevel::Error,
                 "Failed to read bulk data {}: short read from '{}' (expected {} bytes)",
                 bulkId, filePath.string(), fileSize);
            DELTA_ENSURE_MSG(false, "JsonAssetArchive ReadBulkData short read");
            return {};
        }
        return buf;
    }
    catch (const std::exception& ex)
    {
        DLOG(LogSerialization, ELogLevel::Error,
             "Failed to read bulk data {}: unexpected exception '{}' (expected valid bulkDataMap entry)",
             bulkId, ex.what());
        DELTA_ENSURE_MSG(false, "JsonAssetArchive ReadBulkData exception");
        return {};
    }
}
