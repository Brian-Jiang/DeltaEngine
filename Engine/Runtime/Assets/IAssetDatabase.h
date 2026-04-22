#pragma once

#include "EngineIncludes.h"

#include <concepts>
#include <filesystem>

#include "Core/UUID.h"

DELTA_ENGINE_NS_BEGIN

class DObject;
class DPrimaryAsset;

class DELTAENGINE_API IAssetDatabase
{
public:
    virtual ~IAssetDatabase() = default;

    /// Loads an asset and returns it as the requested derived type.
    template <typename T>
        requires std::derived_from<T, DPrimaryAsset>
    T* LoadAsset(const AssetId& id)
    {
        return static_cast<T*>(LoadAsset(id));
    }

    /// Loads an asset and returns a non-owning pointer.
    virtual DPrimaryAsset* LoadAsset(const AssetId& id) = 0;
    /// Returns true when the asset is already loaded.
    virtual bool IsLoaded(const AssetId& id) const = 0;
    /// Finds an object inside a loaded asset.
    virtual DObject* FindObject(const AssetId& assetId, const ObjectId& objId) const = 0;
    /// Resolves an asset id from a source path.
    virtual AssetId FindAssetIdByPath(const std::filesystem::path& path) const = 0;
    /// Persists the asset to disk (writes JSON header/body and bulk sidecars).
    virtual void SaveAsset(const AssetId& id) = 0;
};

DELTA_ENGINE_NS_END
