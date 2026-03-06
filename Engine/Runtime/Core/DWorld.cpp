#include "Core/DWorld.h"

#include <stack>
#include <format>

#include "Core/SceneComponent.h"
#include "Core/Camera.h"
#include "Runtime/Core/GameObject.h"
#include "Graphics/Renderer/Renderer.h"
#include "Graphics/Light/LightComponent.h"
#include "Reflection/ReflectionRegistry.h"
#include "Reflection/DClass.h"

using namespace DeltaEngine;

DWorld::DWorld()
    : m_rootSceneComponent(nullptr)
    , m_gameObjectsChanged(false)
{
    SceneComponent* sceneComponent = CreateDObject<SceneComponent>();
    m_rootSceneComponent = sceneComponent;
}

GameObject* DWorld::CreateGameObject(const std::string& name)
{
    GameObject* gameObject = CreateDObject<GameObject>();
    gameObject->m_name = name;
    gameObject->m_currentWorld = this;
    m_gameObjects.push_back(gameObject);
    m_gameObjectsChanged = true;
    return gameObject;
}

GameObject* DWorld::CreateGameObjectByClass(const DClass* dclass)
{
    if (!dclass)
        return nullptr;
    GameObject* gameObject = GetReflectionRegistry().CreateObject<GameObject>(dclass->GetName());
    if (!gameObject)
        return nullptr;
    gameObject->m_name = std::format("New {}", dclass->GetName());
    gameObject->m_currentWorld = this;
    m_gameObjects.push_back(gameObject);
    m_gameObjectsChanged = true;
    return gameObject;
}

void DWorld::DestroyGameObject(GameObject* gameObject)
{
    if (!gameObject)
        return;
    auto it = std::find(m_gameObjects.begin(), m_gameObjects.end(), gameObject);
    if (it != m_gameObjects.end())
    {
        GameObject* obj = *it;
        obj->Destroy();
        m_gameObjects.erase(it);
        m_gameObjectsChanged = true;
        GetReflectionRegistry().DestroyObject(obj);
    }
}

void DeltaEngine::DWorld::InitRenderers(std::shared_ptr<DXGraphicsContext> context) const
{
    std::stack<SceneComponent*> stack;
    stack.push(m_rootSceneComponent);

    while (!stack.empty()) {
        SceneComponent* current = stack.top();
        stack.pop();

        if (auto renderer = dynamic_cast<Renderer*>(current))
        {
            renderer->InitGraphicState(context);
        }

        size_t childCount = current->m_children.size();
        for (int i = static_cast<int>(childCount) - 1; i >= 0; --i) {
            SceneComponent* child = current->m_children[i];
            stack.push(child);
        }
    }
}

void DeltaEngine::DWorld::PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) const
{
    std::stack<SceneComponent*> stack;
    stack.push(m_rootSceneComponent);
    bool hasCamera = false;

    while (!stack.empty())
    {
        SceneComponent* current = stack.top();
        stack.pop();

        // Gather draw calls.
        if (!hasCamera)
        {
            if (Camera* camera = dynamic_cast<Camera*>(current))
            {
                camera->PreGatherDrawCalls(context);
                // todo support multiple cameras and render targets in the future.
                hasCamera = true;
            }
        }

        if (LightComponent* light = dynamic_cast<LightComponent*>(current))
        {
            light->PreGatherDrawCalls(context);
        }

        size_t childCount = current->m_children.size();
        for (int i = static_cast<int>(childCount) - 1; i >= 0; --i)
        {
            SceneComponent* child = current->m_children[i];
            stack.push(child);
        }
    }
}

void DeltaEngine::DWorld::GatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) const
{
    std::stack<SceneComponent*> stack;
    stack.push(m_rootSceneComponent);
    
    while (!stack.empty())
    {
        SceneComponent* current = stack.top();
        stack.pop();

        // Gather draw calls.
        if (Renderer* renderer = dynamic_cast<Renderer*>(current))
        {
            renderer->GatherDrawCalls(context);
        }

        size_t childCount = current->m_children.size();
        for (int i = static_cast<int>(childCount) - 1; i >= 0; --i)
        {
            SceneComponent* child = current->m_children[i];
            stack.push(child);
        }
    }
}

void DeltaEngine::DWorld::PreTick(float deltaTime)
{
    m_gameObjectsChanged = false;
}

void DeltaEngine::DWorld::Clear()
{
    for (GameObject* gameObject : m_gameObjects)
    {
        gameObject->Destroy();
    }

    m_gameObjects.clear();
}

SceneComponent* DeltaEngine::DWorld::GetRootSceneComponent() const { return m_rootSceneComponent; }

const std::vector<GameObject*>& DeltaEngine::DWorld::GetGameObjects() const { return m_gameObjects; }

bool DeltaEngine::DWorld::IsGameObjectsChanged() const { return m_gameObjectsChanged; }

DWorld* DeltaEngine::DWorld::CreateWorld()
{
    DWorld* world = CreateDObject<DWorld>();
    return world;
}
