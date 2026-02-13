#include "Core/DWorld.h"

#include <stack>

#include "Core/SceneComponent.h"
#include "Runtime/Core/GameObject.h"
#include "Graphics/Renderer/Renderer.h"

using namespace DeltaEngine;

DeltaEngine::DWorld::DWorld()
{
    std::shared_ptr<SceneComponent> sceneComponent = std::make_shared<SceneComponent>();
    m_rootSceneComponent = sceneComponent;
}

std::shared_ptr<GameObject> DeltaEngine::DWorld::CreateGameObject()
{
    std::shared_ptr<GameObject> gameObject = std::make_shared<GameObject>();
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

        if (auto renderer = dynamic_cast<Renderer*>(current.get())) {
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

    while (!stack.empty()) {
        std::shared_ptr<SceneComponent> current = stack.top();
        stack.pop();

        // Gather draw calls.
        if (auto renderer = dynamic_cast<Renderer*>(current.get())) {
            renderer->GatherDrawCalls(context);
        }

        size_t childCount = current->m_children.size();
        for (int i = static_cast<int>(childCount) - 1; i >= 0; --i) {
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
        if (auto renderer = dynamic_cast<Renderer*>(current.get())) {
            renderer->GatherDrawCalls(context);
        }

        size_t childCount = current->m_children.size();
        for (int i = static_cast<int>(childCount) - 1; i >= 0; --i) {
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
