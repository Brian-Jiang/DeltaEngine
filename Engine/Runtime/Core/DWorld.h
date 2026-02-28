#pragma once

#include "EngineIncludes.h"

#include <memory>
#include <vector>
#include <string>

#include "Core/DObject.h"
#include "Runtime/Graphics/DXGraphicsContext.h"

#include "DWorld.generated.h"

DELTA_ENGINE_NS_BEGIN

class SceneComponent;
class GameObject;

DCLASS()
class DWorld : public DObject, public std::enable_shared_from_this<DWorld>
{
    DGENERATED_BODY(DWorld)

public:
    DELTAENGINE_API DWorld();

    DFUNCTION()
    DELTAENGINE_API std::shared_ptr<GameObject> CreateGameObject(const std::string& name = "New GameObject");

    /// Walk the scene tree and call InitGraphicState on every Renderer.
    DFUNCTION()
    DELTAENGINE_API void InitRenderers(std::shared_ptr<DXGraphicsContext> context) const;

    DFUNCTION()
    DELTAENGINE_API void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) const;

    /// Walk the scene tree and call GatherDrawCalls on every Renderer.
    DFUNCTION()
    DELTAENGINE_API void GatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) const;

    DFUNCTION()
    DELTAENGINE_API void PreTick(float deltaTime);

    DFUNCTION()
    std::shared_ptr<SceneComponent> GetRootSceneComponent() const { return m_rootSceneComponent; }

    /// Clears all game objects and breaks the circular reference between the world and game objects.
    DFUNCTION()
    DELTAENGINE_API void Clear();

    DFUNCTION()
    const std::vector<std::shared_ptr<GameObject>>& GetGameObjects() const { return m_gameObjects; }
    DFUNCTION()
    inline bool IsGameObjectsChanged() const { return m_gameObjectsChanged; }

    DFUNCTION()
    DELTAENGINE_API static std::shared_ptr<DWorld> CreateWorld();

private:
    DPROPERTY()
    std::shared_ptr<SceneComponent> m_rootSceneComponent;
    DPROPERTY()
    std::vector<std::shared_ptr<GameObject>> m_gameObjects;
    DPROPERTY()
    bool m_gameObjectsChanged = false;
};

DELTA_ENGINE_NS_END
