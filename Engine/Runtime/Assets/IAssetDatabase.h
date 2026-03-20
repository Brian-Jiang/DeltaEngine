#pragma once

#include "EngineIncludes.h"

#include <filesystem>

#include "Core/UUID.h"

DELTA_ENGINE_NS_BEGIN

class DObject;
class DPrimaryAsset;

class DELTAENGINE_API IAssetDatabase
{
public:
    virtual ~IAssetDatabase() = default;

    template <typename T>
        requires std::derived_from<T, DPrimaryAsset>
    T* LoadAsset(const AssetId& id)
    {
        static_assert(std::is_base_of_v<DPrimaryAsset, T>, "T must derive from DPrimaryAsset");
        return static_cast<T*>(LoadAsset(id));
    }

    // Returns a non-owning pointer. Lifetime is managed by the implementation.
    virtual DPrimaryAsset* LoadAsset(const AssetId& id) = 0;
    virtual bool IsLoaded(const AssetId& id) const = 0;
    virtual DObject* FindObject(const AssetId& assetId, const ObjectId& objId) const = 0;
    virtual AssetId FindAssetIdByPath(const std::filesystem::path& path) const = 0;
};

DELTA_ENGINE_NS_END
