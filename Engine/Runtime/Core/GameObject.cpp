#include "Core/GameObject.h"

#include "Core/DWorld.h"

using namespace DeltaEngine;

GameObject::GameObject()
{
	//AddComponent<Transform>();
}

GameObject::~GameObject()
{
	//for (std::shared_ptr<Component> component : m_components)
	//{
	//	delete component;
	//}
}

template <typename T> requires IsComponent<T>
std::shared_ptr<Component> GameObject::AddComponent()
{
	std::shared_ptr<Component> component = std::make_shared<T>();
	m_components.push_back(component);
	return component;
}

template<typename T> requires IsSceneComponent<T>
std::shared_ptr<SceneComponent> DeltaEngine::GameObject::AddSceneComponent() {
	std::shared_ptr<SceneComponent> sceneComponent = std::make_shared<T>();
    m_sceneComponents.push_back(sceneComponent);
    if (m_rootSceneComponent.expired()) {
        m_rootSceneComponent = sceneComponent;
		auto worldSceneRoot = m_currentWorld->GetRootSceneComponent();
        if (worldSceneRoot) {
            sceneComponent->SetParent(worldSceneRoot);
        }
	}
	else {
        sceneComponent->SetParent(m_rootSceneComponent.lock());
	}

    return sceneComponent;
}
