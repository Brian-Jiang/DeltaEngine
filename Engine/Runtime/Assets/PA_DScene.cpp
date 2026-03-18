#include "Assets/PA_DScene.h"

#include "Assets/DPrimaryAsset.h"
#include "Core/DScene.h"
#include "Core/UUID.h"
#include "Reflection/ReflectionRegistry.h"

using namespace DeltaEngine;

DPrimaryAsset* PA_DScene::Create(const std::string& sceneName)
{
    DPrimaryAsset* asset = CreateDObject<DPrimaryAsset>();
    asset->GetHeader().m_persistentId = UUID::Generate();
    asset->GetHeader().m_className    = "DPrimaryAsset";

    DScene* scene = CreateDObject<DScene>();
    scene->SetObjectId(UUID::Generate());
    scene->SetName(sceneName);

    // Transfer ownership to the primary asset.
    // The shared_ptr deleter calls the reflection registry to properly
    // destroy the object, mirroring the pattern used in DeserializeBody.
    asset->AddObject(std::shared_ptr<DObject>(scene, [](DObject* p)
    {
        GetReflectionRegistry().DestroyObject(p);
    }));

    return asset;
}

DScene* PA_DScene::GetScene(DPrimaryAsset* asset)
{
    if (!asset)
        return nullptr;

    for (const auto& obj : asset->GetObjects())
    {
        if (DScene* scene = dynamic_cast<DScene*>(obj.get()))
            return scene;
    }
    return nullptr;
}
