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
class DClass;

DCLASS()
class DWorld : public DObject
{
    DGENERATED_BODY(DWorld)

public:
    DELTAENGINE_API DWorld();

    DFUNCTION()
    DELTAENGINE_API GameObject* CreateGameObject(const std::string& name = "New GameObject");

    DELTAENGINE_API GameObject* CreateGameObjectByClass(const DClass* dclass,
                                                        const std::string& name = "New GameObject");

    /// Walk the scene tree and call InitGraphicState on every Renderer.
    DELTAENGINE_API void InitRenderers(std::shared_ptr<DXGraphicsContext> context) const;

    DELTAENGINE_API void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) const;

    /// Walk the scene tree and call GatherDrawCalls on every Renderer.
    DELTAENGINE_API void GatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) const;

    DFUNCTION()
    DELTAENGINE_API void PreTick(float deltaTime);

    DFUNCTION()
    DELTAENGINE_API SceneComponent* GetRootSceneComponent() const;

    /// Clears all game objects and breaks the circular reference between the world and game objects.
    DFUNCTION()
    DELTAENGINE_API void Clear();

    DFUNCTION()
    DELTAENGINE_API const std::vector<GameObject*>& GetGameObjects() const;
    DFUNCTION()
    DELTAENGINE_API bool IsGameObjectsChanged() const;

    DFUNCTION()
    DELTAENGINE_API static DWorld* CreateWorld();

private:
    SceneComponent* m_rootSceneComponent;
    std::vector<GameObject*> m_gameObjects;
    bool m_gameObjectsChanged = false;
};

DELTA_ENGINE_NS_END
