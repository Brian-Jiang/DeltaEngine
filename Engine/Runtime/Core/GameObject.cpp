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

void DeltaEngine::GameObject::Destroy() {

}

//template <typename T> requires IsComponent<T>
//std::shared_ptr<T> GameObject::AddComponent()
//{
//	std::shared_ptr<T> component = std::make_shared<T>();
//	m_components.push_back(component);
//	return component;
//}

//template<typename T> requires IsSceneComponent<T>
//std::shared_ptr<T> DeltaEngine::GameObject::AddSceneComponent()
//{
//	std::shared_ptr<T> sceneComponent = std::make_shared<T>();
//    m_sceneComponents.push_back(sceneComponent);
//    if (m_rootSceneComponent.expired()) {
//        m_rootSceneComponent = sceneComponent;
//		auto worldSceneRoot = m_currentWorld->GetRootSceneComponent();
//        if (worldSceneRoot) {
//            sceneComponent->SetParent(worldSceneRoot);
//        }
//	}
//	else {
//        sceneComponent->SetParent(m_rootSceneComponent.lock());
//	}
//
//    return sceneComponent;
//}
