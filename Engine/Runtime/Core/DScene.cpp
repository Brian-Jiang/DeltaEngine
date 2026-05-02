#include "Core/DScene.h"

#include <algorithm>

#include "Core/GameObject.h"

using namespace DeltaEngine;

DScene::DScene()
    : m_name("New Scene")
{
}

void DScene::AddGameObject(GameObject* go)
{
    if (!go)
        return;
    if (std::find(m_gameObjects.begin(), m_gameObjects.end(), go) != m_gameObjects.end())
    {
        DLOG(LogCore, ELogLevel::Warning,
            "DScene::AddGameObject: GameObject '{}' ({}) already in scene '{}' — ignoring duplicate add",
            go->GetName(), static_cast<void*>(go), m_name);
        return;
    }
    m_gameObjects.push_back(go);
    MarkDirty();
}

void DScene::RemoveGameObject(GameObject* go)
{
    if (!go)
        return;
    auto it = std::find(m_gameObjects.begin(), m_gameObjects.end(), go);
    if (it != m_gameObjects.end())
    {
        m_gameObjects.erase(it);
        MarkDirty();
    }
    else
        DLOG(LogCore, ELogLevel::Warning,
            "DScene::RemoveGameObject: GameObject '{}' ({}) not found in scene '{}' — no-op",
            go->GetName(), static_cast<void*>(go), m_name);
}

void DScene::AddComponent(DComponent* component)
{
    if (!component)
        return;
    if (std::find(m_components.begin(), m_components.end(), component) != m_components.end())
    {
        DLOG(LogCore, ELogLevel::Warning,
            "DScene::AddComponent: component '{}' ({}) already tracked by scene '{}' — ignoring duplicate add",
            component->GetName(), static_cast<void*>(component), m_name);
        return;
    }
    m_components.push_back(component);
    MarkDirty();
}

void DScene::RemoveComponent(DComponent* component)
{
    if (!component)
        return;
    auto it = std::find(m_components.begin(), m_components.end(), component);
    if (it != m_components.end())
    {
        m_components.erase(it);
        MarkDirty();
    }
    else
        DLOG(LogCore, ELogLevel::Warning,
            "DScene::RemoveComponent: component '{}' ({}) not found in scene '{}' — no-op",
            component->GetName(), static_cast<void*>(component), m_name);
}
