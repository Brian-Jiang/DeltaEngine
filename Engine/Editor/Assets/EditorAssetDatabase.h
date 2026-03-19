#pragma once

#include "EditorIncludes.h"

#include "Runtime/Assets/IAssetDatabase.h"
#include "Runtime/Core/UUID.h"
#include "Runtime/Assets/DPrimaryAsset.h"

#include <filesystem>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

DELTA_ENGINE_NS_BEGIN

class DELTAEDITOR_API EditorAssetDatabase : public IAssetDatabase
{
public:
    enum class AssetState
    {
        Unregistered,
        HeaderOnly,
        Loading,
        Loaded
    };

    void ScanAssetsFolder(const std::filesystem::path& root);

    const DPrimaryAsset::Header* GetAssetHeader(const AssetId& id) const;
    std::filesystem::path        GetAssetPath(const AssetId& id) const;

    DPrimaryAsset* LoadAsset(const AssetId& id) override;

    DObject* FindObject(const AssetId& assetId, const ObjectId& objId) const override;

    void SaveDirtyAssets();
    void SaveAsset(const AssetId& id);

    void CreateAsset(const std::filesystem::path& filePath, DPrimaryAsset* asset);
    void CreateAsset(const std::filesystem::path& filePath, DObject* object);

    bool       IsLoaded(const AssetId& id) const override;
    AssetState GetState(const AssetId& id) const;

    /// Returns the AssetId registered for the given file path,
    /// or a null UUID if no matching entry is found.
    AssetId FindAssetIdByPath(const std::filesystem::path& path) const override;

private:
    void LoadAssetRecursive(const AssetId& id);
    void ResolvePendingBatch();

    static DPrimaryAsset::Header ReadAssetHeaderFromFile(
        const std::filesystem::path& path, bool isJson);

    struct AssetEntry
    {
        DPrimaryAsset::Header          m_header;
        std::filesystem::path          m_filePath;
        AssetState                     m_state = AssetState::HeaderOnly;
        DPrimaryAsset*                 m_instance;
    };

    std::unordered_map<AssetId, AssetEntry> m_assets;
    std::unordered_set<AssetId>             m_currentlyLoading;
    std::vector<AssetId>                    m_newlyLoadedBatch;
};

DELTA_ENGINE_NS_END
