#pragma once

#include "EngineIncludes.h"

#include "Runtime/Core/DObject.h"
#include "Runtime/Graphics/Shadow/ShadowView.h"

#include <memory>
#include <string>
#include <vector>

#include "DWorld.generated.h"

DELTA_ENGINE_NS_BEGIN

class SceneComponent;
class GameObject;
class DClass;
class DScene;
class Skybox;
struct DXGraphicsContext;

DCLASS()
class DWorld : public DObject
{
    DGENERATED_BODY(DWorld)

public:
    DELTAENGINE_API DWorld();

    DFUNCTION()
    DELTAENGINE_API GameObject* CreateGameObject(const std::string& name = "New GameObject");

    DELTAENGINE_API GameObject* CreateGameObjectByClass(const DClass* dclass);

    DELTAENGINE_API void DestroyGameObject(GameObject* gameObject);

    /// Walk the scene tree and call InitGraphicState on every Renderer.
    DELTAENGINE_API void InitRenderers(std::shared_ptr<DXGraphicsContext> context) const;

    DELTAENGINE_API void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) const;

    /// Walk the scene tree and call GatherDrawCalls on every Renderer (excludes skybox).
    DELTAENGINE_API void GatherOpaqueDrawCalls(std::shared_ptr<DXGraphicsContext> context) const;

    /// Walk the scene tree, collect transparent submeshes, sort back-to-front, and draw them.
    DELTAENGINE_API void GatherTransparentDrawCalls(std::shared_ptr<DXGraphicsContext> context) const;

    /// Walk the scene tree and call GatherDrawCalls on every Renderer, then skybox last.
    DELTAENGINE_API void GatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) const;

    DELTAENGINE_API void GatherShadowViews(std::shared_ptr<DXGraphicsContext> context, std::vector<ShadowView>& outViews) const;

    DELTAENGINE_API void GatherShadowDrawCalls(std::shared_ptr<DXGraphicsContext> context, const ShadowView& view) const;

    DFUNCTION()
    DELTAENGINE_API void PreTick(float deltaTime);

    DFUNCTION()
    DELTAENGINE_API SceneComponent* GetRootSceneComponent() const;

    /// Clears all game objects and breaks the circular reference between the world and game objects.
    DFUNCTION()
    DELTAENGINE_API void Clear();

    /// Destroys every GameObject in the world (used before reloading a scene from disk).
    DELTAENGINE_API void DestroyAllWorldGameObjects();

    /// Removes runtime world references to scene-owned GameObjects without destroying them.
    DELTAENGINE_API void DetachAllWorldGameObjects();

    DFUNCTION()
    DELTAENGINE_API const std::vector<GameObject*>& GetGameObjects() const;
    DFUNCTION()
    DELTAENGINE_API bool IsGameObjectsChanged() const;

    DFUNCTION()
    DELTAENGINE_API static DWorld* CreateWorld();

    /// Creates a scene-owned game object, or a temporary one when scene is null.
    DELTAENGINE_API GameObject* CreateGameObjectInScene(DScene* scene, const std::string& name = "New GameObject");

    /// Creates a scene-owned reflected game object, or a temporary one when scene is null.
    DELTAENGINE_API GameObject* CreateGameObjectInScene(DScene* scene, const DClass* dclass);

    /// Adds a loaded scene object to this world's runtime hierarchy.
    DELTAENGINE_API void AddGameObjectFromScene(GameObject* go);

    /// Sets the scene targeted by editor scene-object operations.
    DELTAENGINE_API void SetActiveScene(DScene* scene);

    /// Returns the active scene targeted by editor scene-object operations.
    DFUNCTION()
    DELTAENGINE_API DScene* GetActiveScene() const;

    DELTAENGINE_API void SetSkybox(Skybox* skybox);
    DELTAENGINE_API Skybox* GetSkybox() const;

private:
    DPROPERTY(HideInDetails)
    SceneComponent* m_rootSceneComponent;

    DPROPERTY(HideInDetails)
    std::vector<GameObject*> m_gameObjects;

    bool m_gameObjectsChanged = false;

    /// The scene that editor scene-object operations target.
    DScene* m_activeScene = nullptr;

    /// Runtime skybox pointer, synced from the active scene.
    DPROPERTY(HideInDetails)
    Skybox* m_skybox = nullptr;
};

DELTA_ENGINE_NS_END
