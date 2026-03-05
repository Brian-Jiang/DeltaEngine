#include "Core/GameObject.h"

#include "Reflection/ReflectionRegistry.h"
#include "Reflection/DClass.h"

#include "Core/DWorld.h"
#include "Core/SceneComponent.h"

using namespace DeltaEngine;

GameObject::GameObject()
    : m_name("New GameObject")
    , m_rootSceneComponent(nullptr)
    , m_currentWorld(nullptr)
{
	//AddComponent<Transform>();
}

//DeltaEngine::GameObject::GameObject(const std::string& name)
//    : m_name(name)
//{
//}

GameObject::~GameObject()
{
	
}

void GameObject::Destroy()
{
    for (DComponent* component : m_components)
    {
        GetReflectionRegistry().DestroyObject(component);
    }

    for (SceneComponent* sceneComponent : m_sceneComponents)
    {
        GetReflectionRegistry().DestroyObject(sceneComponent);
    }
}

DComponent* GameObject::AddComponentByClass(const DClass* dclass)
{
    if (!dclass)
        return nullptr;

    auto& registry = GetReflectionRegistry();
    const DClass* sceneCompClass = registry.FindClassByName("SceneComponent");

    if (sceneCompClass && dclass->IsChildOf(sceneCompClass))
    {
        SceneComponent* sc = registry.CreateObject<SceneComponent>(dclass->GetName());
        if (!sc)
            return nullptr;
        sc->RegisterComponent(this);
        sc->SetName("New Scene Component");
        m_sceneComponents.push_back(sc);
        if (!m_rootSceneComponent)
        {
            m_rootSceneComponent = sc;
            if (m_currentWorld)
            {
                SceneComponent* worldRoot = m_currentWorld->GetRootSceneComponent();
                if (worldRoot)
                    sc->SetParent(worldRoot);
            }
        }
        else
        {
            sc->SetParent(m_rootSceneComponent);
        }
        return sc;
    }
    else
    {
        DComponent* comp = registry.CreateObject<DComponent>(dclass->GetName());
        if (!comp)
            return nullptr;
        comp->RegisterComponent(this);
        comp->SetName("New Component");
        m_components.push_back(comp);
        return comp;
    }
}

const std::string& GameObject::GetName() const { return m_name; }

DWorld* GameObject::GetCurrentWorld() const { return m_currentWorld; }

SceneComponent* GameObject::GetRootSceneComponent() const { return m_rootSceneComponent; }

const std::vector<SceneComponent*>& GameObject::GetSceneComponents() const { return m_sceneComponents; }

const std::vector<DComponent*>& GameObject::GetComponents() const { return m_components; }