#include "Editor/Assets/EditorAssetDatabase.h"

#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Serialization/JsonAssetArchive.h"

#include <nlohmann/json.hpp>
#include <fstream>

using namespace DeltaEngine;

// ---------------------------------------------------------------------------
// ScanAssetsFolder
// ---------------------------------------------------------------------------

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
            if (isBinary)
            {
                existingIt->second.m_header   = header;
                existingIt->second.m_filePath  = path;
            }
            continue;
        }

        m_assets[header.m_persistentId] = AssetEntry{
            .m_header   = header,
            .m_filePath = path,
            .m_state    = AssetState::HeaderOnly,
            .m_instance = nullptr
        };
    }
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
                if (h.contains("assetId"))
                    header.m_persistentId = UUID::FromString(h["assetId"].get<std::string>());
            }
        }
        catch (...)
        {
            header.m_magic = 0;
        }
    }
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
            return;
        }

        nlohmann::json root;
        try
        {
            root = nlohmann::json::parse(file);
        }
        catch (...)
        {
            m_currentlyLoading.erase(id);
            return;
        }

        auto asset = CreateDObject<DPrimaryAsset>();

        if (root.contains("header"))
        {
            JsonAssetArchive headerAr(root["header"], entry.m_filePath.parent_path());
            asset->SerializeHeader(headerAr);
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
    }

    m_currentlyLoading.erase(id);
    m_newlyLoadedBatch.push_back(id);
}

// ---------------------------------------------------------------------------
// ResolvePendingBatch (Phase 2)
// ---------------------------------------------------------------------------

void EditorAssetDatabase::ResolvePendingBatch()
{
    auto resolveProps = [&](auto& self, DStruct* ds, DObject* obj) -> void {
        if (!ds) return;
        if (DStruct* parent = ds->GetSuper())
            self(self, parent, obj);
        for (DProperty* prop = ds->GetOwnProperties(); prop; prop = prop->GetNext())
        {
            auto* ptrProp = dynamic_cast<DObjectPtrPropertyBase*>(prop);
            if (!ptrProp)
                continue;

            ScriptPointer sp = ptrProp->GetUnresolvedPointer(obj);
            if (sp.IsNull())
                continue;

            DObject* resolved = FindObject(sp.m_assetId, sp.m_objectId);
            ptrProp->ResolvePointer(obj, resolved);
        }
    };

    for (const auto& assetId : m_newlyLoadedBatch)
    {
        auto it = m_assets.find(assetId);
        if (it == m_assets.end() || !it->second.m_instance)
            continue;

        auto& asset = it->second.m_instance;
        for (auto& obj : asset->GetObjects())
            resolveProps(resolveProps, obj->GetClass(), obj.get());
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

    SaveAsset(newId);
}

void EditorAssetDatabase::CreateAsset(const std::filesystem::path& filePath, DObject* object)
{
    if (!object)
        return;

    auto asset = CreateDObject<DPrimaryAsset>();
    asset->GetHeader().m_persistentId = UUID::Generate();
    asset->GetHeader().m_className = "DPrimaryAsset";
    asset->AddObject(std::shared_ptr<DObject>(object));
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
