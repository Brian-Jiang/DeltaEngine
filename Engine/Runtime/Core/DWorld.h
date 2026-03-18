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
class DScene;

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

    // ---- Scene integration ----

    /// Creates a GameObject that belongs to the given scene (persisted on save)
    /// and is immediately active in this world.
    /// If scene is null the GameObject is temporary (no owning asset).
    DELTAENGINE_API GameObject* CreateGameObjectInScene(DScene* scene, const std::string& name = "New GameObject");

    /// Creates a scene-owned GameObject of a specific reflected class.
    /// The GO name is derived from the class name.
    /// If scene is null the GameObject is temporary (no owning asset).
    DELTAENGINE_API GameObject* CreateGameObjectInScene(DScene* scene, const DClass* dclass);

    /// Adds a GameObject that was loaded from a DScene into this world's
    /// runtime hierarchy. Reconstructs SceneComponent parent links and
    /// updates world transforms. Does not transfer ownership.
    DELTAENGINE_API void AddGameObjectFromScene(GameObject* go);

    /// Sets the active scene. New GameObjects added via CreateGameObjectInScene
    /// without an explicit scene argument default to this scene.
    DELTAENGINE_API void SetActiveScene(DScene* scene);

    DFUNCTION()
    DELTAENGINE_API DScene* GetActiveScene() const;

private:
    SceneComponent* m_rootSceneComponent;
    std::vector<GameObject*> m_gameObjects;
    bool m_gameObjectsChanged = false;

    /// The scene that "Add GameObject" operations target in the editor.
    /// Set automatically when the first scene is loaded.
    DScene* m_activeScene = nullptr;
};

DELTA_ENGINE_NS_END
