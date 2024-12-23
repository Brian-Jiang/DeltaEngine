#include "Core/DWorld.h"

#include "Core/SceneComponent.h"

using namespace DeltaEngine;

DeltaEngine::DWorld::DWorld() {
    std::shared_ptr<SceneComponent> sceneComponent = std::make_shared<SceneComponent>();
    m_rootSceneComponent = sceneComponent;
}

std::shared_ptr<DWorld> DeltaEngine::DWorld::CreateWorld() {
    std::shared_ptr<DWorld> world = std::make_shared<DWorld>();
    return world;
}
