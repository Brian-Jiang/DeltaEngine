#pragma once

#include "EngineIncludes.h"

#include <vector>
#include <memory>
#include <string>

#include "Core/DObject.h"
#include "Core/DComponent.h"
#include "Core/SceneComponent.h"
#include "Core/DWorld.h"

#include "GameObject.generated.h"

DELTA_ENGINE_NS_BEGIN

DCLASS()
class GameObject: public DObject, public std::enable_shared_from_this<GameObject>
{
    DGENERATED_BODY(GameObject)
    friend class DWorld;

public:
	DELTAENGINE_API GameObject();
	GameObject(const std::string& name) = delete;

	DELTAENGINE_API void Initialize(const std::string& name);

	DELTAENGINE_API ~GameObject();

	template <typename T> requires IsDComponent<T>
	std::shared_ptr<T> AddComponent(std::string name = "New Component")
    {
        std::shared_ptr<T> component = std::shared_ptr<T>(CreateDObject<T>());
        if (component)
        {
            component->Initialize(name, shared_from_this());
        }
        m_components.push_back(component);
        return component;
	}

	template <typename T> requires IsSceneComponent<T>
    std::shared_ptr<T> AddSceneComponent(std::string name = "New Scene Component")
    {
        std::shared_ptr<T> sceneComponent = std::shared_ptr<T>(CreateDObject<T>());
        if (sceneComponent)
        {
            sceneComponent->Initialize(name, shared_from_this());
        }
        m_sceneComponents.push_back(sceneComponent);
        if (m_rootSceneComponent.expired())
        {
            m_rootSceneComponent = sceneComponent;
            auto worldSceneRoot = m_currentWorld->GetRootSceneComponent();
            if (worldSceneRoot)
            {
                sceneComponent->SetParent(worldSceneRoot);
            }
        }
        else
        {
            sceneComponent->SetParent(m_rootSceneComponent.lock());
        }

        return sceneComponent;
    }

    DFUNCTION()
    DELTAENGINE_API void Destroy();

    DFUNCTION()
    const std::string& GetName() const;

    DFUNCTION()
    std::shared_ptr<DWorld> GetCurrentWorld() const;
    DFUNCTION()
    std::shared_ptr<SceneComponent> GetRootSceneComponent() const;
    DFUNCTION()
    const std::vector<std::shared_ptr<SceneComponent>>& GetSceneComponents() const;
    DFUNCTION()
    const std::vector<std::shared_ptr<DComponent>>& GetComponents() const;

    template <typename T> requires IsSceneComponent<T>
    inline std::shared_ptr<T> GetRootSceneComponent() const
    {
        return std::static_pointer_cast<T>(m_rootSceneComponent.lock()); 
    }

private:
    DPROPERTY()
    std::string m_name;
    DPROPERTY()
    std::weak_ptr<SceneComponent> m_rootSceneComponent;
    DPROPERTY()
    std::vector<std::shared_ptr<SceneComponent>> m_sceneComponents;
    DPROPERTY()
	std::vector<std::shared_ptr<DComponent>> m_components;
    DPROPERTY()
	std::shared_ptr<DWorld> m_currentWorld;
};

DELTA_ENGINE_NS_END
