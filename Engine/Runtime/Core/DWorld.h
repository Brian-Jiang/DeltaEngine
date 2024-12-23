#pragma once

#include "EngineIncludes.h"

#include <memory>

#include "Core/DObject.h"

DELTA_ENGINE_NS_BEGIN

class SceneComponent;

class DWorld : public DObject
{

public:
    DWorld();
    std::shared_ptr<SceneComponent> GetRootSceneComponent() const { return m_rootSceneComponent; }

    static std::shared_ptr<DWorld> CreateWorld();

private:
    std::shared_ptr<SceneComponent> m_rootSceneComponent;
};

DELTA_ENGINE_NS_END