#pragma once

#include "EditorIncludes.h"

#include "Runtime/Assets/IAssetDatabase.h"
#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Core/GC/StrongDObjectPtr.h"
#include "Runtime/Core/UUID.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

DELTA_ENGINE_NS_BEGIN

class DELTAEDITOR_API EditorAssetDatabase : public IAssetDatabase
{
public:
    ~EditorAssetDatabase();

    enum class AssetState
    {
        Unregistered,
        HeaderOnly,
        Loading,
        Loaded
    };

    struct AssetEntry
    {
        DPrimaryAsset::Header m_header;
        std::filesystem::path m_filePath;
        AssetState m_state = AssetState::HeaderOnly;
        StrongDObjectPtr<DPrimaryAsset> m_instance;
        nlohmann::json m_meta;
    };

    using IAssetDatabase::LoadAsset;

    void ScanAssetsFolder(const std::filesystem::path& root);

    /// Drops the in-memory instance and reloads the asset body from disk (JSON path only).
    void ReloadAssetFromDisk(const AssetId& id);
    const std::unordered_map<AssetId, AssetEntry>& GetAllAssets() const;

    const DPrimaryAsset::Header* GetAssetHeader(const AssetId& id) const;
    std::filesystem::path        GetAssetPath(const AssetId& id) const;
    DPrimaryAsset*               GetLoadedAsset(const AssetId& id) const;

    /// Returns the cached `{static, dynamic}` metadata blob for the asset, or an empty
    /// object if the id is unknown. Always shape `{static:{}, dynamic:{desc, tags, ...}}`.
    const nlohmann::json& GetAssetMeta(const AssetId& id) const;

    /// Rebuilds the cached meta blob for an asset from its live in-memory instance.
    /// No-op if the asset has no loaded instance.
    void RefreshAssetMetaCache(const AssetId& id);

    DPrimaryAsset* LoadAsset(const AssetId& id) override;

    DObject* FindObject(const AssetId& assetId, const ObjectId& objId) const override;

    void SaveDirtyAssets();
    void SaveAsset(const AssetId& id) override;
    AssetId DuplicateAsset(const AssetId& id);
    bool    DeleteAsset(const AssetId& id);

    /// Renames the `.dasset.json` and bulk sidecars; resolves `desiredStem` to a unique filename
    /// (`name`, `name_1`, …) in the asset directory. Returns false on failure.
    bool RenameAssetToStem(const AssetId& id, const std::string& desiredStem, std::string* outFinalStem = nullptr);

    /// Moves the asset file to `exactStem.dasset.json` in the same folder; fails if that path exists
    /// (other than the asset's current path).
    bool RenameAssetToExactStem(const AssetId& id, const std::string& exactStem);

    /// Moves the asset's .dasset.json and all bulk sidecars into targetFolder.
    /// Returns false if the asset is not found or the filesystem move fails.
    bool MoveAsset(const AssetId& id, const std::filesystem::path& targetFolder);

    void CreateAsset(const std::filesystem::path& filePath, DPrimaryAsset* asset);
    void CreateAsset(const std::filesystem::path& filePath, DObject* object);

    /** Imports source files and returns all created asset IDs. */
    std::vector<AssetId> ImportAssets(const std::vector<std::filesystem::path>& sourcePaths);

    bool       IsLoaded(const AssetId& id) const override;
    AssetState GetState(const AssetId& id) const;

    /// Returns the AssetId registered for the given file path,
    /// or a null UUID if no matching entry is found.
    AssetId FindAssetIdByPath(const std::filesystem::path& path) const override;

    /// Monotonic counter bumped whenever the asset id-set or any asset file path changes.
    /// UI can compare it to detect structural changes without rescanning.
    uint64_t GetAssetSetRevision() const { return m_assetSetRevision; }

    /// Signals a structural change (e.g. an empty folder created/removed on disk) that does
    /// not pass through the asset-map mutators. Bumps GetAssetSetRevision().
    void BumpAssetSetRevision() { ++m_assetSetRevision; }

private:
    void LoadAssetRecursive(const AssetId& id);
    void ResolvePendingBatch();
    DPrimaryAsset* CreateAssetInstance(const std::string& className);

    static DPrimaryAsset::Header ReadAssetHeaderFromFile(
        const std::filesystem::path& path, bool isJson);

    /// Opens `.dasset.json`, reads only `root["meta"]`, applies dynamic-meta backfill, and
    /// returns the resulting `{static, dynamic}` blob. Body and bulk are not parsed.
    static nlohmann::json ReadAssetMetaFromFile(const std::filesystem::path& path);

    std::unordered_map<AssetId, AssetEntry> m_assets;
    std::unordered_map<std::filesystem::path, AssetId> m_assetPathMap;

    std::unordered_set<AssetId>             m_currentlyLoading;
    std::vector<AssetId>                    m_newlyLoadedBatch;

    uint64_t                                m_assetSetRevision = 0;
};

DELTA_ENGINE_NS_END
