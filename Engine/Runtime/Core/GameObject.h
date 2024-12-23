#pragma once

#include "EngineIncludes.h"

#include <vector>
#include <memory>

#include "Core/DObject.h"
#include "Core/Component.h"
#include "Core/SceneComponent.h"

DELTA_ENGINE_NS_BEGIN

class DWorld;

class GameObject: public DObject
{
public:
	GameObject();
	~GameObject();

	template <typename T> requires IsComponent<T>
	std::shared_ptr<Component> AddComponent();

	template <typename T> requires IsSceneComponent<T>
    std::shared_ptr<SceneComponent> AddSceneComponent();

	std::shared_ptr<DWorld> GetCurrentWorld() const { return m_currentWorld; }
    std::shared_ptr<SceneComponent> GetRootSceneComponent() const { return m_rootSceneComponent.lock(); }

private:
    std::weak_ptr<SceneComponent> m_rootSceneComponent;
    std::vector<std::shared_ptr<SceneComponent>> m_sceneComponents;
	std::vector<std::shared_ptr<Component>> m_components;
	std::shared_ptr<DWorld> m_currentWorld;
};

DELTA_ENGINE_NS_END
