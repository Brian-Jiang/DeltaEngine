#include "Core/GameObject.h"

#include <algorithm>
#include <format>
#include <queue>

#include "Assets/DPrimaryAsset.h"
#include "Reflection/DClass.h"
#include "Reflection/ReflectionRegistry.h"

using namespace DeltaEngine;

GameObject::GameObject()
    : m_name("New GameObject")
    , m_rootSceneComponent(nullptr)
    , m_currentWorld(nullptr)
{
}

GameObject::~GameObject()
{
}

void GameObject::Destroy()
{
    for (DComponent* component : m_components)
    {
        RemoveComponent(component);
    }

    if (m_rootSceneComponent)
    {
        RemoveComponent(m_rootSceneComponent);
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

        if (HasOwningAsset())
        {
            GetOwningAsset()->AddObject(sc);
        }

        sc->RegisterComponent(this);
        sc->SetName(std::format("New {}", dclass->GetName()));
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

    DComponent* comp = registry.CreateObject<DComponent>(dclass->GetName());
    if (!comp)
        return nullptr;

    if (HasOwningAsset())
        GetOwningAsset()->AddObject(comp);

    comp->RegisterComponent(this);
    comp->SetName(std::format("New {}", dclass->GetName()));
    m_components.push_back(comp);
    return comp;
}

void GameObject::RemoveComponent(DComponent* component)
{
    if (!component)
        return;

    auto it = std::find(m_components.begin(), m_components.end(), component);
    if (it != m_components.end())
    {
        component->MarkForDestroy();
        m_components.erase(it);
        GetReflectionRegistry().DestroyObject(component);
        return;
    }

    auto scIt = std::find(m_sceneComponents.begin(), m_sceneComponents.end(), component);
    if (scIt != m_sceneComponents.end())
    {
        std::queue<SceneComponent*> componentQueue;
        std::vector<SceneComponent*> componentsToRemove;
        componentQueue.push(*scIt);
        while (!componentQueue.empty())
        {
            SceneComponent* current = componentQueue.front();
            componentQueue.pop();
            componentsToRemove.push_back(current);
            for (SceneComponent* child : current->GetChildren())
            {
                componentQueue.push(child);
            }
        }

        std::reverse(componentsToRemove.begin(), componentsToRemove.end());
        for (SceneComponent* comp : componentsToRemove)
        {
            if (comp == m_rootSceneComponent)
            {
                m_rootSceneComponent = nullptr;
            }

            comp->MarkForDestroy();
            comp->SetParent(nullptr);
            std::erase(m_sceneComponents, comp);
            GetReflectionRegistry().DestroyObject(comp);
        }
    }
}

const std::string& GameObject::GetName() const { return m_name; }

DWorld* GameObject::GetCurrentWorld() const { return m_currentWorld; }

SceneComponent* GameObject::GetRootSceneComponent() const { return m_rootSceneComponent; }

const std::vector<SceneComponent*>& GameObject::GetSceneComponents() const { return m_sceneComponents; }

const std::vector<DComponent*>& GameObject::GetComponents() const { return m_components; }
