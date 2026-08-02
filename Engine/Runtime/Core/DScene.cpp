#include "Runtime/Core/DScene.h"

#include "Runtime/Core/GameObject.h"

#include <algorithm>

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
