#include "Runtime/Assets/PA_DScene.h"

#include "Runtime/Core/DScene.h"
#include "Runtime/Core/UUID.h"

using namespace DeltaEngine;

PA_DScene* PA_DScene::Create(const std::string& sceneName)
{
    PA_DScene* asset = CreateDObject<PA_DScene>();
    DELTA_VERIFY_MSG(asset != nullptr, "PA_DScene::Create failed to allocate PA_DScene");

    asset->GetHeader().m_persistentId = UUID::Generate();
    asset->GetHeader().m_className = "PA_DScene";

    DScene* scene = CreateDObject<DScene>();
    DELTA_VERIFY_MSG(scene != nullptr, "PA_DScene::Create failed to allocate DScene");

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
