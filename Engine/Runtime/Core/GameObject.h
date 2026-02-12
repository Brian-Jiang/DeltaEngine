#pragma once

#include "EngineIncludes.h"

#include <vector>
#include <memory>

#include "Core/DObject.h"
#include "Core/DComponent.h"
#include "Core/SceneComponent.h"
#include "Core/DWorld.h"

DELTA_ENGINE_NS_BEGIN

class GameObject: public DObject
{
    friend class DWorld;

public:
	GameObject();
	~GameObject();

	template <typename T> requires IsDComponent<T>
	std::shared_ptr<T> AddComponent()
    {
        std::shared_ptr<T> component = std::make_shared<T>();
        m_components.push_back(component);
        return component;
    }

	template <typename T> requires IsSceneComponent<T>
    std::shared_ptr<T> AddSceneComponent()
    {
        std::shared_ptr<T> sceneComponent = std::make_shared<T>();
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

    void Destroy();

	inline std::shared_ptr<DWorld> GetCurrentWorld() const { return m_currentWorld; }
    std::shared_ptr<SceneComponent> GetRootSceneComponent() const { return m_rootSceneComponent.lock(); }

private:
    std::weak_ptr<SceneComponent> m_rootSceneComponent;
    std::vector<std::shared_ptr<SceneComponent>> m_sceneComponents;
	std::vector<std::shared_ptr<DComponent>> m_components;
	std::shared_ptr<DWorld> m_currentWorld;
};

DELTA_ENGINE_NS_END
