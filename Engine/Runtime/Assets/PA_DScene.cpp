#include "Assets/PA_DScene.h"

#include "Core/DScene.h"
#include "Core/UUID.h"

using namespace DeltaEngine;

PA_DScene* PA_DScene::Create(const std::string& sceneName)
{
    PA_DScene* asset = CreateDObject<PA_DScene>();
    asset->GetHeader().m_persistentId = UUID::Generate();
    asset->GetHeader().m_className = "PA_DScene";

    DScene* scene = CreateDObject<DScene>();
    scene->SetName(sceneName);
    asset->AddObject(scene);

    return asset;
}

DScene* PA_DScene::GetScene() const
{
    for (DObject* object : GetObjects())
    {
        if (DScene* scene = dynamic_cast<DScene*>(object))
            return scene;
    }

    return nullptr;
}
