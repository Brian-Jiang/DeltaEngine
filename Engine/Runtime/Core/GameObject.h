#pragma once

#include "EngineIncludes.h"

#include <vector>
#include <memory>
#include <string>

#include "Core/DObject.h"
#include "Core/DComponent.h"
#include "Core/SceneComponent.h"
#include "Core/DWorld.h"

DELTA_ENGINE_NS_BEGIN

class GameObject: public DObject, public std::enable_shared_from_this<GameObject>
{
    friend class DWorld;

public:
	DELTAENGINE_API GameObject();
	DELTAENGINE_API GameObject(const std::string& name);
	DELTAENGINE_API ~GameObject();

	template <typename T> requires IsDComponent<T>
	std::shared_ptr<T> AddComponent(std::string name = "New Component")
    {
        std::shared_ptr<T> component = std::make_shared<T>(name, shared_from_this());
        m_components.push_back(component);
        return component;
    }

	template <typename T> requires IsSceneComponent<T>
    std::shared_ptr<T> AddSceneComponent(std::string name = "New Scene Component")
    {
        std::shared_ptr<T> sceneComponent = std::make_shared<T>(name, shared_from_this());
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

    DELTAENGINE_API void Destroy();

    const std::string& GetName() const { return m_name; }

    inline std::shared_ptr<DWorld> GetCurrentWorld() const { return m_currentWorld; }
    inline std::shared_ptr<SceneComponent> GetRootSceneComponent() const { return m_rootSceneComponent.lock(); }
    inline const std::vector<std::shared_ptr<SceneComponent>>& GetSceneComponents() const { return m_sceneComponents; }
    inline const std::vector<std::shared_ptr<DComponent>>& GetComponents() const { return m_components; }

    template <typename T> requires IsSceneComponent<T>
    inline std::shared_ptr<T> GetRootSceneComponent() const
    {
        return std::static_pointer_cast<T>(m_rootSceneComponent.lock()); 
    }

private:
    std::string m_name;
    std::weak_ptr<SceneComponent> m_rootSceneComponent;
    std::vector<std::shared_ptr<SceneComponent>> m_sceneComponents;
	std::vector<std::shared_ptr<DComponent>> m_components;
	std::shared_ptr<DWorld> m_currentWorld;
};

DELTA_ENGINE_NS_END
