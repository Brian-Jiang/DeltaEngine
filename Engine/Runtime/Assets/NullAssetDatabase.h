#pragma once

#include "Assets/IAssetDatabase.h"

#include <iostream>

DELTA_ENGINE_NS_BEGIN

class NullAssetDatabase final : public IAssetDatabase
{
public:
    NullAssetDatabase() = default;
    ~NullAssetDatabase() override = default;

    DPrimaryAsset* LoadAsset(const AssetId& id) override
    {
        (void)id;
        std::cerr << "[AssetDatabase] NullAssetDatabase: no IAssetDatabase registered - LoadAsset returning nullptr\n";
        return nullptr;
    }

    bool IsLoaded(const AssetId& id) const override
    {
        (void)id;
        std::cerr << "[AssetDatabase] NullAssetDatabase: no IAssetDatabase registered - IsLoaded returning false\n";
        return false;
    }

    DObject* FindObject(const AssetId& assetId, const ObjectId& objId) const override
    {
        (void)assetId;
        (void)objId;
        std::cerr << "[AssetDatabase] NullAssetDatabase: no IAssetDatabase registered - FindObject returning nullptr\n";
        return nullptr;
    }
};

DELTA_ENGINE_NS_END
