#pragma once

#include "Runtime/Assets/IAssetDatabase.h"

DELTA_ENGINE_NS_BEGIN

class DELTAENGINE_API NullAssetDatabase final : public IAssetDatabase
{
public:
    NullAssetDatabase() = default;
    ~NullAssetDatabase() override = default;

    DPrimaryAsset* LoadAsset(const AssetId& id) override;
    bool           IsLoaded(const AssetId& id) const override;
    DObject*       FindObject(const AssetId& assetId, const ObjectId& objId) const override;
    AssetId        FindAssetIdByPath(const std::filesystem::path& path) const override;
    void           SaveAsset(const AssetId& id) override;
};

DELTA_ENGINE_NS_END
