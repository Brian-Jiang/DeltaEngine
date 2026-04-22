#pragma once

#include "Assets/IAssetDatabase.h"

#include <iostream>

DELTA_ENGINE_NS_BEGIN

/// Fallback asset database that logs calls and returns empty results.
class NullAssetDatabase final : public IAssetDatabase
{
public:
    NullAssetDatabase() = default;
    ~NullAssetDatabase() override = default;

    /// Always returns nullptr.
    DPrimaryAsset* LoadAsset(const AssetId& id) override
    {
        (void)id;
        std::cerr << "[AssetDatabase] NullAssetDatabase: no IAssetDatabase registered - LoadAsset returning nullptr\n";
        return nullptr;
    }

    /// Always returns false.
    bool IsLoaded(const AssetId& id) const override
    {
        (void)id;
        std::cerr << "[AssetDatabase] NullAssetDatabase: no IAssetDatabase registered - IsLoaded returning false\n";
        return false;
    }

    /// Always returns nullptr.
    DObject* FindObject(const AssetId& assetId, const ObjectId& objId) const override
    {
        (void)assetId;
        (void)objId;
        std::cerr << "[AssetDatabase] NullAssetDatabase: no IAssetDatabase registered - FindObject returning nullptr\n";
        return nullptr;
    }

    /// Always returns a null asset id.
    AssetId FindAssetIdByPath(const std::filesystem::path& path) const override
    {
        (void)path;
        std::cerr << "[AssetDatabase] NullAssetDatabase: no IAssetDatabase registered - FindAssetIdByPath returning null AssetId\n";
        return AssetId::Null();
    }

    /// No-op; logs a warning.
    void SaveAsset(const AssetId& id) override
    {
        (void)id;
        std::cerr << "[AssetDatabase] NullAssetDatabase: no IAssetDatabase registered - SaveAsset ignored\n";
    }
};

DELTA_ENGINE_NS_END
