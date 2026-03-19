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
}

void DScene::AddComponent(DComponent* component)
{
    if (!component)
        return;
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
}
