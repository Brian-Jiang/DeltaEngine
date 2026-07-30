#include "Runtime/Core/GameObject.h"

#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Logging/LogChannels.h"

#include <algorithm>
#include <format>
#include <queue>

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
    const auto components = m_components;
    for (DComponent* component : components)
        RemoveComponent(component);

    if (m_rootSceneComponent)
        RemoveComponent(m_rootSceneComponent);
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
        {
            DLOG(LogCore, ELogLevel::Error,
                "AddComponentByClass: reflection failed creating SceneComponent-derived '{}' on GameObject '{}'",
                dclass->GetName(), m_name);
            return nullptr;
        }

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
    {
        DLOG(LogCore, ELogLevel::Error,
            "AddComponentByClass: reflection failed creating DComponent-derived '{}' on GameObject '{}'",
            dclass->GetName(), m_name);
        return nullptr;
    }

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
        }
    }
}

void GameObject::DetachComponent(DComponent* component)
{
    if (!component)
        return;

    auto it = std::find(m_components.begin(), m_components.end(), component);
    if (it != m_components.end())
    {
        component->MarkForDestroy();
        m_components.erase(it);
        return;
    }

    auto* sc = dynamic_cast<SceneComponent*>(component);
    if (!sc)
        return;

    auto scIt = std::find(m_sceneComponents.begin(), m_sceneComponents.end(), sc);
    if (scIt != m_sceneComponents.end())
    {
        if (sc == m_rootSceneComponent)
            m_rootSceneComponent = nullptr;

        sc->MarkForDestroy();
        sc->SetParent(nullptr);
        m_sceneComponents.erase(scIt);
    }
}

void GameObject::InsertComponent(DComponent* comp, int index)
{
    if (!comp)
        return;

    if (!comp->HasOwningAsset() && HasOwningAsset())
        GetOwningAsset()->AddObject(comp);

    comp->RegisterComponent(this);

    int idx = std::clamp(index, 0, static_cast<int>(m_components.size()));
    m_components.insert(m_components.begin() + idx, comp);
}

void GameObject::InsertSceneComponent(SceneComponent* sc, int index, SceneComponent* parent)
{
    if (!sc)
        return;

    if (!sc->HasOwningAsset() && HasOwningAsset())
        GetOwningAsset()->AddObject(sc);

    sc->RegisterComponent(this);

    int idx = std::clamp(index, 0, static_cast<int>(m_sceneComponents.size()));
    m_sceneComponents.insert(m_sceneComponents.begin() + idx, sc);

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
    else if (parent)
    {
        sc->SetParent(parent);
    }
    else
    {
        sc->SetParent(m_rootSceneComponent);
    }
}

const std::string& GameObject::GetName() const { return m_name; }

DWorld* GameObject::GetCurrentWorld() const { return m_currentWorld; }

SceneComponent* GameObject::GetRootSceneComponent() const { return m_rootSceneComponent; }

const std::vector<SceneComponent*>& GameObject::GetSceneComponents() const { return m_sceneComponents; }

const std::vector<DComponent*>& GameObject::GetComponents() const { return m_components; }
