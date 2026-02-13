#pragma once

#include "EngineIncludes.h"

#include <memory>
#include <vector>

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

    /// Walk the scene tree and call InitGraphicState on every Renderer.
    void InitRenderers(std::shared_ptr<DXGraphicsContext> context) const;

    void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) const;

    /// Walk the scene tree and call GatherDrawCalls on every Renderer.
    void GatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) const;

    std::shared_ptr<SceneComponent> GetRootSceneComponent() const { return m_rootSceneComponent; }

    /// Clears all game objects and breaks the circular reference between the world and game objects.
    void Clear();

    static std::shared_ptr<DWorld> CreateWorld();

private:
    std::shared_ptr<SceneComponent> m_rootSceneComponent;
    std::vector<std::shared_ptr<GameObject>> m_gameObjects;
};

DELTA_ENGINE_NS_END
