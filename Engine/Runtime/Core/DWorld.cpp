#include "Core/DWorld.h"

#include <stack>

#include "Core/SceneComponent.h"
#include "Core/Camera.h"
#include "Runtime/Core/GameObject.h"
#include "Graphics/Renderer/Renderer.h"
#include "Graphics/Light/LightComponent.h"

using namespace DeltaEngine;

DeltaEngine::DWorld::DWorld()
{
    std::shared_ptr<SceneComponent> sceneComponent = std::make_shared<SceneComponent>();
    m_rootSceneComponent = sceneComponent;
}

std::shared_ptr<GameObject> DWorld::CreateGameObject(const std::string& name)
{
    std::shared_ptr<GameObject> gameObject = std::make_shared<GameObject>(name);
    gameObject->m_currentWorld = shared_from_this();
    m_gameObjects.push_back(gameObject);
    return gameObject;
}

void DeltaEngine::DWorld::InitRenderers(std::shared_ptr<DXGraphicsContext> context) const
{
    std::stack<std::shared_ptr<SceneComponent>> stack;
    stack.push(m_rootSceneComponent);

    while (!stack.empty()) {
        std::shared_ptr<SceneComponent> current = stack.top();
        stack.pop();

        if (auto renderer = dynamic_cast<Renderer*>(current.get()))
        {
            renderer->InitGraphicState(context);
        }

        size_t childCount = current->m_children.size();
        for (int i = static_cast<int>(childCount) - 1; i >= 0; --i) {
            std::shared_ptr<SceneComponent> child = current->m_children[i];
            stack.push(child);
        }
    }
}

void DeltaEngine::DWorld::PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) const
{
    std::stack<std::shared_ptr<SceneComponent>> stack;
    stack.push(m_rootSceneComponent);
    bool hasCamera = false;

    while (!stack.empty())
    {
        std::shared_ptr<SceneComponent> current = stack.top();
        stack.pop();

        // Gather draw calls.
        if (!hasCamera)
        {
            if (std::shared_ptr<Camera> camera = std::dynamic_pointer_cast<Camera>(current))
            {
                camera->PreGatherDrawCalls(context);
                // todo support multiple cameras and render targets in the future.
                hasCamera = true;
            }
        }

        if (std::shared_ptr<LightComponent> light = std::dynamic_pointer_cast<LightComponent>(current))
        {
            light->PreGatherDrawCalls(context);
        }

        size_t childCount = current->m_children.size();
        for (int i = static_cast<int>(childCount) - 1; i >= 0; --i)
        {
            std::shared_ptr<SceneComponent> child = current->m_children[i];
            stack.push(child);
        }
    }
}

void DeltaEngine::DWorld::GatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) const
{
    std::stack<std::shared_ptr<SceneComponent>> stack;
    stack.push(m_rootSceneComponent);
    
    while (!stack.empty())
    {
        std::shared_ptr<SceneComponent> current = stack.top();
        stack.pop();

        // Gather draw calls.
        if (std::shared_ptr<Renderer> renderer = std::dynamic_pointer_cast<Renderer>(current))
        {
            renderer->GatherDrawCalls(context);
        }

        size_t childCount = current->m_children.size();
        for (int i = static_cast<int>(childCount) - 1; i >= 0; --i)
        {
            std::shared_ptr<SceneComponent> child = current->m_children[i];
            stack.push(child);
        }
    }
}

void DeltaEngine::DWorld::Clear()
{
    m_gameObjects.clear();
}

std::shared_ptr<DWorld> DeltaEngine::DWorld::CreateWorld()
{
    std::shared_ptr<DWorld> world = std::make_shared<DWorld>();
    return world;
}
