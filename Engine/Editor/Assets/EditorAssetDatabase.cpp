#include "Editor/Assets/EditorAssetDatabase.h"

#include "Editor/Assets/AssetImporter.h"

#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DObjectReferenceTraversal.h"
#include "Runtime/Core/DObject.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Serialization/JsonAssetArchive.h"
#include "Serialization/ISerializationCallbackReceiver.h"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>

using namespace DeltaEngine;

// ---------------------------------------------------------------------------
// ScanAssetsFolder
// ---------------------------------------------------------------------------

void EditorAssetDatabase::ReloadAssetFromDisk(const AssetId& id)
{
    auto it = m_assets.find(id);
    if (it == m_assets.end())
        return;

    AssetEntry& entry = it->second;
    if (entry.m_instance)
    {
        DPrimaryAsset* pa = entry.m_instance;
        std::vector<DObject*> owned = pa->GetObjects();
        for (DObject* obj : owned)
            GetReflectionRegistry().DestroyObject(obj);
        GetReflectionRegistry().DestroyObject(pa);
        entry.m_instance = nullptr;
    }
    entry.m_state = AssetState::HeaderOnly;
    LoadAsset(id);
}

void EditorAssetDatabase::ScanAssetsFolder(const std::filesystem::path& root)
{
    for (auto& entry : std::filesystem::recursive_directory_iterator(root))
    {
        if (!entry.is_regular_file())
            continue;

        auto& path = entry.path();
        auto ext  = path.extension().string();
        auto stem = path.stem().string();

        bool isBinary = (ext == ".dasset");
        bool isJson   = (ext == ".json" && stem.ends_with(".dasset"));
        if (!isBinary && !isJson)
            continue;

        auto header = ReadAssetHeaderFromFile(path, isJson);
        if (header.m_magic != 0x444C5441)
            continue;

        auto existingIt = m_assets.find(header.m_persistentId);
        if (existingIt != m_assets.end())
        {
            existingIt->second.m_header   = header;
            existingIt->second.m_filePath = path;
            continue;
        }

        m_assets[header.m_persistentId] = AssetEntry{
            .m_header   = header,
            .m_filePath = path,
            .m_state    = AssetState::HeaderOnly,
            .m_instance = nullptr
        };

        m_assetPathMap[path] = header.m_persistentId;
    }
}

const std::unordered_map<AssetId, EditorAssetDatabase::AssetEntry>& EditorAssetDatabase::GetAllAssets() const
{
    return m_assets;

    // todo sorting?
    //std::vector<AssetInfo> assets;
    //assets.reserve(m_assets.size());

    //for (const auto& [assetId, entry] : m_assets)
    //{
    //    assets.push_back(AssetInfo{
    //        .m_assetId  = assetId,
    //        .m_header   = entry.m_header,
    //        .m_filePath = entry.m_filePath,
    //        .m_state    = entry.m_state,
    //    });
    //}

    //std::sort(assets.begin(), assets.end(), [](const AssetInfo& lhs, const AssetInfo& rhs)
    //{
    //    return lhs.m_filePath.generic_string() < rhs.m_filePath.generic_string();
    //});

    //return assets;
}

// ---------------------------------------------------------------------------
// ReadAssetHeaderFromFile
// ---------------------------------------------------------------------------

DPrimaryAsset::Header EditorAssetDatabase::ReadAssetHeaderFromFile(
    const std::filesystem::path& path, bool isJson)
{
    DPrimaryAsset::Header header;

    if (isJson)
    {
        std::ifstream file(path);
        if (!file.is_open())
            return header;

        try
        {
            nlohmann::json root = nlohmann::json::parse(file);
            if (root.contains("header"))
            {
                auto& h = root["header"];
                std::string magic = h.value("magic", "");
                header.m_magic       = (magic == "DLTA") ? 0x444C5441 : 0;
                header.m_fileVersion = h.value("version", 0u);
                header.m_className   = h.value("className", "");
                header.m_persistentId = UUID::FromString(h["assetId"].get<std::string>());
            }
        }
        catch (std::exception& e)
        {
            printf("Failed to read asset header from JSON file '%s': %s\n", path.string().c_str(), e.what());
            header.m_magic = 0;
        }
    }
    // todo binary header reading
    else
    {
        header.m_magic = 0;
    }

    return header;
}

// ---------------------------------------------------------------------------
// LoadAsset (public)
// ---------------------------------------------------------------------------

DPrimaryAsset* EditorAssetDatabase::LoadAsset(const AssetId& id)
{
    auto it = m_assets.find(id);
    if (it == m_assets.end())
        return nullptr;

    if (it->second.m_state == AssetState::Loaded)
        return it->second.m_instance;

    m_newlyLoadedBatch.clear();
    LoadAssetRecursive(id);
    ResolvePendingBatch();

    return m_assets[id].m_instance;
}

// ---------------------------------------------------------------------------
// LoadAssetRecursive (Phase 1)
// ---------------------------------------------------------------------------

DPrimaryAsset* EditorAssetDatabase::CreateAssetInstance(const std::string& className)
{
    if (className.empty() || className == "DPrimaryAsset")
        return CreateDObject<DPrimaryAsset>();

    DObject* object = GetReflectionRegistry().CreateObject(className);
    if (!object)
        return CreateDObject<DPrimaryAsset>();

    DPrimaryAsset* asset = dynamic_cast<DPrimaryAsset*>(object);
    if (!asset) {
        GetReflectionRegistry().DestroyObject(object);
        return CreateDObject<DPrimaryAsset>();
    }

    return asset;
}

void EditorAssetDatabase::LoadAssetRecursive(const AssetId& id)
{
    auto it = m_assets.find(id);
    if (it == m_assets.end())
        return;

    auto& entry = it->second;
    if (entry.m_state == AssetState::Loaded)
        return;
    if (m_currentlyLoading.contains(id))
        return;

    m_currentlyLoading.insert(id);

    bool isJson = entry.m_filePath.string().ends_with(".dasset.json");

    if (isJson)
    {
        std::ifstream file(entry.m_filePath);
        if (!file.is_open())
        {
            m_currentlyLoading.erase(id);
            printf("Failed to open asset file '%s'\n", entry.m_filePath.string().c_str());
            return;
        }

        nlohmann::json root;
        try
        {
            root = nlohmann::json::parse(file);
        }
        catch (std::exception& e)
        {
            m_currentlyLoading.erase(id);
            printf("Failed to parse JSON file '%s': %s\n", entry.m_filePath.string().c_str(), e.what());
            return;
        }

        auto asset = CreateAssetInstance(entry.m_header.m_className);

        if (root.contains("header"))
        {
            JsonAssetArchive headerAr(root["header"], entry.m_filePath.parent_path());
            asset->SerializeHeader(headerAr);
        }
        else
        {
            m_currentlyLoading.erase(id);
            printf("JSON file '%s' does not contain 'header' object\n", entry.m_filePath.string().c_str());
            return;
        }

        JsonAssetArchive bodyAr(root, entry.m_filePath.parent_path());
        asset->SerializeBody(bodyAr);

        // Phase 2: load bulk data payloads (handles were read from JSON by SerializeBody)
        JsonAssetArchive bulkAr(root, entry.m_filePath.parent_path());
        asset->SerializeBulkData(bulkAr);

        auto refs = asset->CollectExternalReferences();
        for (const auto& sp : refs)
        {
            if (!sp.m_assetId.IsNull() && sp.m_assetId != id)
                LoadAssetRecursive(sp.m_assetId);
        }

        entry.m_state    = AssetState::Loaded;
        entry.m_instance = asset;

        for (auto& obj : asset->GetObjects())
        {
            if (auto* callbackReceiver = dynamic_cast<ISerializationCallbackReceiver*>(obj))
            {
                callbackReceiver->OnAfterDeserialize();
            }
        }
    }
    // todo binary loading

    m_currentlyLoading.erase(id);
    m_newlyLoadedBatch.push_back(id);
}

// ---------------------------------------------------------------------------
// ResolvePendingBatch (Phase 2)
// ---------------------------------------------------------------------------

void EditorAssetDatabase::ResolvePendingBatch()
{
    for (const auto& assetId : m_newlyLoadedBatch)
    {
        auto it = m_assets.find(assetId);
        if (it == m_assets.end() || !it->second.m_instance)
        {
            printf("Unexpected error: asset '%s' not found during resolve phase\n", assetId.ToString().c_str());
            continue;
        }

        auto& asset = it->second.m_instance;
        for (auto& obj : asset->GetObjects())
        {
            VisitUnresolvedObjectReferencesInStruct(obj->GetClass(), obj,
                [&](DObjectPtrPropertyBase* ptrProp, void* valueAddress, const ScriptPointer& sp)
                {
                    DObject* resolved = FindObject(sp.m_assetId, sp.m_objectId);
                    ptrProp->ResolvePointer(valueAddress, resolved);
                });
        }
    }
    m_newlyLoadedBatch.clear();
}

// ---------------------------------------------------------------------------
// FindObject
// ---------------------------------------------------------------------------

DObject* EditorAssetDatabase::FindObject(
    const AssetId& assetId, const ObjectId& objId) const
{
    auto it = m_assets.find(assetId);
    if (it == m_assets.end() || !it->second.m_instance)
        return nullptr;
    return it->second.m_instance->FindObject(objId);
}

// ---------------------------------------------------------------------------
// SaveDirtyAssets / SaveAsset
// ---------------------------------------------------------------------------

void EditorAssetDatabase::SaveDirtyAssets()
{
    for (auto& [id, entry] : m_assets)
    {
        if (entry.m_state != AssetState::Loaded)
            continue;
        if (!entry.m_instance || !entry.m_instance->IsDirty())
            continue;
        SaveAsset(id);
    }
}

void EditorAssetDatabase::SaveAsset(const AssetId& id)
{
    auto it = m_assets.find(id);
    if (it == m_assets.end())
        return;

    auto& entry = it->second;
    auto& asset = entry.m_instance;
    if (!asset)
        return;

    for (auto& obj : asset->GetObjects())
    {
        if (auto* callbackReceiver = dynamic_cast<ISerializationCallbackReceiver*>(obj))
            callbackReceiver->OnBeforeSerialize();
    }

    bool isJson = entry.m_filePath.string().ends_with(".dasset.json");

    if (isJson)
    {
        const auto assetDir  = entry.m_filePath.parent_path();
        const auto assetStem = entry.m_filePath.stem().stem().string();

        // Phase 1: assign bulk IDs and write sidecar .bin files
        JsonAssetArchive bulkAr(assetDir, assetStem);
        asset->SerializeBulkData(bulkAr);

        // Phase 2: serialize header and body JSON (bulk handles now have correct IDs)
        JsonAssetArchive headerAr;
        asset->SerializeHeader(headerAr);

        JsonAssetArchive bodyAr;
        asset->SerializeBody(bodyAr);

        nlohmann::json output;
        output["header"] = headerAr.GetRoot();
        const auto& bulkRoot = bulkAr.GetRoot();
        if (bulkRoot.contains("header") && bulkRoot["header"].contains("bulkDataMap"))
            output["header"]["bulkDataMap"] = bulkRoot["header"]["bulkDataMap"];
        for (auto& [key, val] : bodyAr.GetRoot().items())
            output[key] = val;

        std::ofstream out(entry.m_filePath);
        out << output.dump(2);
    }

    asset->ClearDirty();
}

AssetId EditorAssetDatabase::DuplicateAsset(const AssetId& id)
{
    DPrimaryAsset* asset = LoadAsset(id);
    if (!asset)
        return AssetId::Null();

    auto it = m_assets.find(id);
    if (it == m_assets.end())
        return AssetId::Null();

    const std::filesystem::path sourcePath = it->second.m_filePath;
    const std::filesystem::path assetDir = sourcePath.parent_path();
    const std::string sourceStem = sourcePath.stem().stem().string();

    std::string targetStem = sourceStem + "_duplicated";
    std::filesystem::path targetPath = assetDir / (targetStem + ".dasset.json");
    for (int suffix = 1; std::filesystem::exists(targetPath); ++suffix)
    {
        targetStem = sourceStem + "_duplicated" + std::to_string(suffix);
        targetPath = assetDir / (targetStem + ".dasset.json");
    }

    JsonAssetArchive bulkAr(assetDir, targetStem);
    asset->SerializeBulkData(bulkAr);

    JsonAssetArchive headerAr;
    asset->SerializeHeader(headerAr);

    JsonAssetArchive bodyAr;
    asset->SerializeBody(bodyAr);

    nlohmann::json output;
    output["header"] = headerAr.GetRoot();
    const auto& bulkRoot = bulkAr.GetRoot();
    if (bulkRoot.contains("header") && bulkRoot["header"].contains("bulkDataMap"))
        output["header"]["bulkDataMap"] = bulkRoot["header"]["bulkDataMap"];
    for (auto& [key, val] : bodyAr.GetRoot().items())
        output[key] = val;

    const AssetId newAssetId = UUID::Generate();
    output["header"]["assetId"] = newAssetId.ToString();

    std::unordered_map<std::string, std::string> objectIdRemap;
    if (output.contains("objects") && output["objects"].is_array())
    {
        for (auto& objectJson : output["objects"])
        {
            if (!objectJson.is_object() || !objectJson.contains("_objectId") || !objectJson["_objectId"].is_string())
                continue;

            const std::string oldObjectId = objectJson["_objectId"].get<std::string>();
            const std::string newObjectId = UUID::Generate().ToString();
            objectIdRemap[oldObjectId] = newObjectId;
            objectJson["_objectId"] = newObjectId;
        }
    }

    const std::string oldAssetId = id.ToString();
    const std::string duplicatedAssetId = newAssetId.ToString();
    auto remapScriptPointers = [&](auto& self, nlohmann::json& node) -> void
    {
        if (node.is_object())
        {
            if (node.contains("assetId") && node.contains("objectId") &&
                node["assetId"].is_string() && node["objectId"].is_string())
            {
                const std::string nodeAssetId = node["assetId"].get<std::string>();
                const std::string nodeObjectId = node["objectId"].get<std::string>();
                if (nodeAssetId == oldAssetId)
                {
                    node["assetId"] = duplicatedAssetId;
                    if (auto remapIt = objectIdRemap.find(nodeObjectId); remapIt != objectIdRemap.end())
                        node["objectId"] = remapIt->second;
                }
            }

            for (auto& [key, value] : node.items())
                self(self, value);
        }
        else if (node.is_array())
        {
            for (auto& value : node)
                self(self, value);
        }
    };
    remapScriptPointers(remapScriptPointers, output);

    std::ofstream out(targetPath);
    if (!out.is_open())
        return AssetId::Null();
    out << output.dump(2);

    DPrimaryAsset::Header header = ReadAssetHeaderFromFile(targetPath, true);
    m_assets[newAssetId] = AssetEntry{
        .m_header   = header,
        .m_filePath = targetPath,
        .m_state    = AssetState::HeaderOnly,
        .m_instance = nullptr
    };

    m_assetPathMap[targetPath] = newAssetId;

    return newAssetId;
}

// todo delete existing asset?
bool EditorAssetDatabase::DeleteAsset(const AssetId& id)
{
    auto it = m_assets.find(id);
    if (it == m_assets.end())
        return false;

    const std::filesystem::path assetPath = it->second.m_filePath;
    bool deletedAnything = false;

    if (assetPath.string().ends_with(".dasset.json") && std::filesystem::exists(assetPath))
    {
        try
        {
            std::ifstream file(assetPath);
            if (file.is_open())
            {
                nlohmann::json root = nlohmann::json::parse(file);
                if (root.contains("header") && root["header"].contains("bulkDataMap"))
                {
                    for (auto& [bulkId, bulkEntry] : root["header"]["bulkDataMap"].items())
                    {
                        if (!bulkEntry.is_object() || !bulkEntry.contains("file") || !bulkEntry["file"].is_string())
                            continue;

                        const std::filesystem::path bulkPath = assetPath.parent_path() / bulkEntry["file"].get<std::string>();
                        deletedAnything |= std::filesystem::remove(bulkPath);
                    }
                }
            }
        }
        catch (...)
        {
        }
    }

    deletedAnything |= std::filesystem::remove(assetPath);
    m_assetPathMap.erase(it->second.m_filePath);
    m_assets.erase(it);

    return deletedAnything;
}

namespace
{
void CollectBulkPathsFromJsonFile(
    const std::filesystem::path& assetJsonPath,
    std::vector<std::filesystem::path>& outBulkAbsolute)
{
    outBulkAbsolute.clear();
    if (!assetJsonPath.string().ends_with(".dasset.json") || !std::filesystem::exists(assetJsonPath))
        return;
    try
    {
        std::ifstream file(assetJsonPath);
        if (!file.is_open())
            return;
        nlohmann::json root = nlohmann::json::parse(file);
        if (!root.contains("header") || !root["header"].contains("bulkDataMap"))
            return;
        const auto parent = assetJsonPath.parent_path();
        for (auto& [bulkId, bulkEntry] : root["header"]["bulkDataMap"].items())
        {
            if (!bulkEntry.is_object() || !bulkEntry.contains("file") || !bulkEntry["file"].is_string())
                continue;
            outBulkAbsolute.push_back(parent / bulkEntry["file"].get<std::string>());
        }
    }
    catch (...)
    {
    }
}
} // namespace

bool EditorAssetDatabase::MoveAsset(const AssetId& id, const std::filesystem::path& targetFolder)
{
    auto it = m_assets.find(id);
    if (it == m_assets.end())
        return false;

    const std::filesystem::path oldPath = it->second.m_filePath;
    if (!oldPath.string().ends_with(".dasset.json"))
        return false;

    std::error_code ec;
    const std::filesystem::path absTarget = std::filesystem::absolute(targetFolder, ec);
    if (ec)
        return false;

    if (std::filesystem::equivalent(oldPath.parent_path(), absTarget, ec) && !ec)
        return true;

    const std::filesystem::path newPath = absTarget / oldPath.filename();
    if (std::filesystem::exists(newPath))
        return false;

    std::vector<std::filesystem::path> bulkPaths;
    CollectBulkPathsFromJsonFile(oldPath, bulkPaths);

    std::filesystem::rename(oldPath, newPath, ec);
    if (ec)
        return false;

    for (const auto& bulkPath : bulkPaths)
    {
        if (std::filesystem::exists(bulkPath))
            std::filesystem::rename(bulkPath, absTarget / bulkPath.filename(), ec);
    }

    m_assetPathMap.erase(oldPath);
    it->second.m_filePath = newPath;
    m_assetPathMap[newPath] = id;

    return true;
}

bool EditorAssetDatabase::RenameAssetToExactStem(const AssetId& id, const std::string& exactStem)
{
    if (exactStem.empty())
        return false;

    DPrimaryAsset* asset = LoadAsset(id);
    if (!asset)
        return false;

    auto it = m_assets.find(id);
    if (it == m_assets.end())
        return false;

    std::filesystem::path oldPath = it->second.m_filePath;
    if (!oldPath.string().ends_with(".dasset.json"))
        return false;

    const std::string oldStem = oldPath.stem().stem().string();
    if (exactStem == oldStem)
        return true;

    const std::filesystem::path parent  = oldPath.parent_path();
    const std::filesystem::path newPath = parent / (exactStem + ".dasset.json");

    if (std::filesystem::exists(newPath))
    {
        bool same = false;
        try
        {
            same = std::filesystem::equivalent(newPath, oldPath);
        }
        catch (...)
        {
        }
        if (!same)
            return false;
    }

    std::vector<std::filesystem::path> oldBulkPaths;
    CollectBulkPathsFromJsonFile(oldPath, oldBulkPaths);

    m_assetPathMap.erase(oldPath);
    it->second.m_filePath = newPath;
    m_assetPathMap[newPath] = id;

    asset->MarkDirty();
    SaveAsset(id);

    if (std::filesystem::exists(oldPath))
    {
        try
        {
            if (!std::filesystem::equivalent(oldPath, newPath))
                std::filesystem::remove(oldPath);
        }
        catch (...)
        {
        }
    }

    for (const auto& bulkPath : oldBulkPaths)
    {
        if (std::filesystem::exists(bulkPath))
        {
            try
            {
                std::filesystem::remove(bulkPath);
            }
            catch (...)
            {
            }
        }
    }

    return true;
}

bool EditorAssetDatabase::RenameAssetToStem(const AssetId& id, const std::string& desiredStem, std::string* outFinalStem)
{
    if (desiredStem.empty())
        return false;

    auto it = m_assets.find(id);
    if (it == m_assets.end())
        return false;

    const std::filesystem::path oldPath = it->second.m_filePath;
    if (!oldPath.string().ends_with(".dasset.json"))
        return false;

    const std::filesystem::path parent = oldPath.parent_path();

    std::string candidate = desiredStem;
    for (int n = 0;; ++n)
    {
        const std::filesystem::path testPath = parent / (candidate + ".dasset.json");
        bool ok = !std::filesystem::exists(testPath);
        if (!ok)
        {
            try
            {
                ok = std::filesystem::equivalent(testPath, oldPath);
            }
            catch (...)
            {
                ok = false;
            }
        }
        if (ok)
        {
            if (outFinalStem)
                *outFinalStem = candidate;
            return RenameAssetToExactStem(id, candidate);
        }
        candidate = desiredStem + "_" + std::to_string(n + 1);
    }
}

// todo handle existing asset at filePath?
void EditorAssetDatabase::CreateAsset(const std::filesystem::path& filePath, DPrimaryAsset* asset)
{
    if (!asset)
        return;

    AssetId newId = asset->GetAssetId();
    if (newId.IsNull())
    {
        newId = UUID::Generate();
        asset->GetHeader().m_persistentId = newId;
    }
    
    m_assets[newId] = AssetEntry
    {
        .m_header   = asset->GetHeader(),
        .m_filePath = filePath,
        .m_state    = AssetState::Loaded,
        .m_instance = asset,
    };

    m_assetPathMap[filePath] = newId;

    SaveAsset(newId);
}

void EditorAssetDatabase::CreateAsset(const std::filesystem::path& filePath, DObject* object)
{
    if (!object)
        return;

    auto asset = CreateDObject<DPrimaryAsset>();
    asset->GetHeader().m_persistentId = UUID::Generate();
    asset->GetHeader().m_className = "DPrimaryAsset";
    asset->AddObject(object);
    object->SetOwningAsset(asset);
    CreateAsset(filePath, asset);
}

// ---------------------------------------------------------------------------
// Query methods
// ---------------------------------------------------------------------------

const DPrimaryAsset::Header* EditorAssetDatabase::GetAssetHeader(const AssetId& id) const
{
    auto it = m_assets.find(id);
    return (it != m_assets.end()) ? &it->second.m_header : nullptr;
}

std::filesystem::path EditorAssetDatabase::GetAssetPath(const AssetId& id) const
{
    auto it = m_assets.find(id);
    return (it != m_assets.end()) ? it->second.m_filePath : std::filesystem::path{};
}

DPrimaryAsset* EditorAssetDatabase::GetLoadedAsset(const AssetId& id) const
{
    auto it = m_assets.find(id);
    if (it == m_assets.end() || it->second.m_state != AssetState::Loaded)
        return nullptr;
    return it->second.m_instance;
}

bool EditorAssetDatabase::IsLoaded(const AssetId& id) const
{
    auto it = m_assets.find(id);
    return it != m_assets.end() && it->second.m_state == AssetState::Loaded;
}

EditorAssetDatabase::AssetState EditorAssetDatabase::GetState(const AssetId& id) const
{
    auto it = m_assets.find(id);
    return (it != m_assets.end()) ? it->second.m_state : AssetState::Unregistered;
}

AssetId EditorAssetDatabase::FindAssetIdByPath(const std::filesystem::path& path) const
{
    auto it = m_assetPathMap.find(path);
    if (it != m_assetPathMap.end())
        return it->second;

    return AssetId::Null();
}

std::vector<AssetId> EditorAssetDatabase::ImportAssets(const std::vector<std::filesystem::path>& sourcePaths)
{
    std::vector<AssetId> allIds;
    for (const auto& path : sourcePaths)
    {
        std::vector<AssetId> ids = AssetImporter::ImportFile(path, *this);
        allIds.insert(allIds.end(), ids.begin(), ids.end());
    }
    return allIds;
}
