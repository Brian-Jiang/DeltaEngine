#pragma once

#include "EngineIncludes.h"

#include <memory>

#include "Core/DObject.h"
#include "Runtime/Graphics/DXGraphicsContext.h"

DELTA_ENGINE_NS_BEGIN

class SceneComponent;
class GameObject;

class DWorld : public DObject, public std::enable_shared_from_this<DWorld>
{

public:
    DWorld();

    std::shared_ptr<GameObject> CreateGameObject();
    void GatherDrawCalls(DXGraphicsContext context) const;

    std::shared_ptr<SceneComponent> GetRootSceneComponent() const { return m_rootSceneComponent; }

    static std::shared_ptr<DWorld> CreateWorld();

private:
    std::shared_ptr<SceneComponent> m_rootSceneComponent;
};

DELTA_ENGINE_NS_END