#include "Core/DWorld.h"

#include <algorithm>
#include <format>
#include <functional>
#include <stack>

#include "Core/SceneComponent.h"
#include "Core/Camera.h"
#include "Core/DScene.h"
#include "Core/Skybox.h"
#include "Runtime/Core/GameObject.h"
#include "Assets/DPrimaryAsset.h"
#include "Graphics/Renderer/Renderer.h"
#include "Graphics/Light/LightComponent.h"
#include "Reflection/ReflectionRegistry.h"
#include "Reflection/DClass.h"
#include "Core/UUID.h"

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

    // If this GO belongs to the active scene, remove it there too.
    if (m_activeScene)
        m_activeScene->RemoveGameObject(gameObject);

    auto it = std::find(m_gameObjects.begin(), m_gameObjects.end(), gameObject);
    if (it != m_gameObjects.end())
    {
        GameObject* obj = *it;
        obj->Destroy();
        if (obj->HasOwningAsset())
            obj->GetOwningAsset()->RemoveObject(obj->GetObjectId());
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

    if (m_skybox)
        m_skybox->Initialize();
}

void DWorld::PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) const
{
    std::stack<SceneComponent*> stack;
    stack.push(m_rootSceneComponent);
    bool hasCamera = false;

    while (!stack.empty())
    {
        SceneComponent* current = stack.top();
        stack.pop();

        if (!hasCamera)
        {
            if (Camera* camera = dynamic_cast<Camera*>(current))
            {
                camera->PreGatherDrawCalls(context);
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

void DWorld::GatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) const
{
    std::stack<SceneComponent*> stack;
    stack.push(m_rootSceneComponent);
    
    while (!stack.empty())
    {
        SceneComponent* current = stack.top();
        stack.pop();

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

    // Skybox draws last: LESS_EQUAL depth test lets it fill pixels the scene didn't touch.
    if (m_skybox)
        m_skybox->GatherDrawCalls(context);
}

void DeltaEngine::DWorld::PreTick(float deltaTime)
{
    (void)deltaTime;
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

void DWorld::DestroyAllWorldGameObjects()
{
    while (!m_gameObjects.empty())
        DestroyGameObject(m_gameObjects.front());
}

SceneComponent* DeltaEngine::DWorld::GetRootSceneComponent() const { return m_rootSceneComponent; }

const std::vector<GameObject*>& DeltaEngine::DWorld::GetGameObjects() const { return m_gameObjects; }

bool DeltaEngine::DWorld::IsGameObjectsChanged() const { return m_gameObjectsChanged; }

DWorld* DeltaEngine::DWorld::CreateWorld()
{
    return CreateDObject<DWorld>();
}

GameObject* DWorld::CreateGameObjectInScene(DScene* scene, const std::string& name)
{
    if (!scene)
        return CreateGameObject(name);

    DPrimaryAsset* pa = scene->GetOwningAsset();

    GameObject* go = CreateDObject<GameObject>();
    go->m_name = name;
    go->m_currentWorld = this;
    go->SetObjectId(UUID::Generate());

    if (pa)
        pa->AddObject(go);

    scene->AddGameObject(go);

    m_gameObjects.push_back(go);
    m_gameObjectsChanged = true;

    return go;
}

GameObject* DWorld::CreateGameObjectInScene(DScene* scene, const DClass* dclass)
{
    if (!dclass)
        return nullptr;

    const std::string name = std::format("New {}", dclass->GetName());
    return CreateGameObjectInScene(scene, name);
}

void DWorld::AddGameObjectFromScene(GameObject* go)
{
    if (!go)
        return;

    go->m_currentWorld = this;
    m_gameObjects.push_back(go);

    SceneComponent* rootSC = go->GetRootSceneComponent();
    if (rootSC && m_rootSceneComponent)
    {
        // The child lists were deserialized, so re-link parents without duplicating children.
        rootSC->m_parent = m_rootSceneComponent;
        m_rootSceneComponent->m_children.push_back(rootSC);

        std::function<void(SceneComponent*)> reconstructLinks =
            [&](SceneComponent* sc)
        {
            for (SceneComponent* child : sc->m_children)
            {
                if (child)
                {
                    child->m_parent = sc;
                    reconstructLinks(child);
                }
            }
        };
        reconstructLinks(rootSC);

        rootSC->UpdateTransform();
    }

    m_gameObjectsChanged = true;
}

void DWorld::SetActiveScene(DScene* scene)
{
    m_activeScene = scene;
    m_skybox = scene ? scene->GetSkybox() : nullptr;
}

DScene* DWorld::GetActiveScene() const
{
    return m_activeScene;
}

void DWorld::SetSkybox(Skybox* skybox)
{
    m_skybox = skybox;
}

Skybox* DWorld::GetSkybox() const
{
    return m_skybox;
}
