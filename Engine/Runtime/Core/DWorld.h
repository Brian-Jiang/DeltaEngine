#pragma once

#include "EngineIncludes.h"

#include <memory>
#include <vector>
#include <string>

#include "Core/DObject.h"
#include "Runtime/Graphics/DXGraphicsContext.h"

DELTA_ENGINE_NS_BEGIN

class SceneComponent;
class GameObject;

class DWorld : public DObject, public std::enable_shared_from_this<DWorld>
{

public:
    DELTAENGINE_API DWorld();

    DELTAENGINE_API std::shared_ptr<GameObject> CreateGameObject(const std::string& name = "New GameObject");

    /// Walk the scene tree and call InitGraphicState on every Renderer.
    DELTAENGINE_API void InitRenderers(std::shared_ptr<DXGraphicsContext> context) const;

    DELTAENGINE_API void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) const;

    /// Walk the scene tree and call GatherDrawCalls on every Renderer.
    DELTAENGINE_API void GatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) const;

    DELTAENGINE_API void PreTick(float deltaTime);

    std::shared_ptr<SceneComponent> GetRootSceneComponent() const { return m_rootSceneComponent; }

    /// Clears all game objects and breaks the circular reference between the world and game objects.
    DELTAENGINE_API void Clear();

    const std::vector<std::shared_ptr<GameObject>>& GetGameObjects() const { return m_gameObjects; }
    inline bool IsGameObjectsChanged() const { return m_gameObjectsChanged; }

    DELTAENGINE_API static std::shared_ptr<DWorld> CreateWorld();

private:
    std::shared_ptr<SceneComponent> m_rootSceneComponent;
    std::vector<std::shared_ptr<GameObject>> m_gameObjects;
    bool m_gameObjectsChanged = false;
};

DELTA_ENGINE_NS_END
