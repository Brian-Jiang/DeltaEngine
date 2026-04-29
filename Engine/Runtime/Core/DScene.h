#pragma once

#include "EngineIncludes.h"

#include <string>
#include <vector>

#include "Core/DObject.h"

#include "DScene.generated.h"

DELTA_ENGINE_NS_BEGIN

class GameObject;
class DComponent;
class Skybox;

/// Serializable scene data. Stores the list of GameObjects that belong to this
/// scene. A DScene is typically owned by a PA_DScene primary asset.
///
/// DWorld is the runtime representation; DScene is the serialized form.
/// GameObjects loaded from a DScene are added to a DWorld at load time.
/// DWorld is never saved — only DScene is persisted.
DCLASS()
class DScene : public DObject
{
    DGENERATED_BODY(DScene)

public:
    DELTAENGINE_API DScene();

    DPROPERTY()
    std::string m_name;

    /// All GameObjects that belong to this scene.
    /// These are serialized as intra-asset object pointers.
    DPROPERTY(HideInDetails)
    std::vector<GameObject*> m_gameObjects;

    DPROPERTY(HideInDetails)
    std::vector<DComponent*> m_components;

    /// Optional skybox assigned to this scene. Serialized as an intra-asset pointer.
    DPROPERTY()
    Skybox* m_skybox = nullptr;

    DELTAENGINE_API const std::string& GetName() const { return m_name; }
    DELTAENGINE_API void SetName(const std::string& name) { m_name = name; }

    DELTAENGINE_API const std::vector<GameObject*>& GetGameObjects() const { return m_gameObjects; }
    DELTAENGINE_API const std::vector<DComponent*>& GetComponents() const { return m_components; }

    /// Appends a GameObject to this scene's list.
    DELTAENGINE_API void AddGameObject(GameObject* go);

    /// Removes a GameObject from this scene's list (does not destroy it).
    DELTAENGINE_API void RemoveGameObject(GameObject* go);

    DELTAENGINE_API void AddComponent(DComponent* component);
    DELTAENGINE_API void RemoveComponent(DComponent* component);

    DELTAENGINE_API Skybox* GetSkybox() const { return m_skybox; }
    DELTAENGINE_API void SetSkybox(Skybox* skybox) { m_skybox = skybox; }
};

DELTA_ENGINE_NS_END
