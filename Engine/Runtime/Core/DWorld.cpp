#include "Runtime/Core/DWorld.h"

#include "Runtime/Core/Camera.h"
#include "Runtime/Core/DScene.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Core/Skybox.h"
#include "Runtime/Core/UUID.h"
#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Graphics/DXGraphicsContext.h"
#include "Runtime/Graphics/Light/LightComponent.h"
#include "Runtime/Graphics/RenderProxy/RenderProxy.h"
#include "Runtime/Graphics/Renderer/MeshRenderer.h"
#include "Runtime/Graphics/Renderer/Renderer.h"
#include "Runtime/Graphics/TransparentDrawEntry.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/ReflectionRegistry.h"

#include <algorithm>
#include <format>
#include <functional>
#include <stack>

using namespace DirectX;
using namespace DeltaEngine;

namespace
{
void ForEachRenderer(const SceneComponent* root, const std::function<void(Renderer*)>& visitor)
{
    if (!root)
        return;

    std::stack<const SceneComponent*> stack;
    stack.push(root);

    while (!stack.empty())
    {
        const SceneComponent* current = stack.top();
        stack.pop();

        if (Renderer* renderer = dynamic_cast<Renderer*>(const_cast<SceneComponent*>(current)))
            visitor(renderer);

        const size_t childCount = current->m_children.size();
        for (int i = static_cast<int>(childCount) - 1; i >= 0; --i)
            stack.push(current->m_children[static_cast<size_t>(i)]);
    }
}
}

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
    {
        DLOG(LogCore, ELogLevel::Error,
            "CreateGameObjectByClass: reflection failed for class '{}' (expected GameObject-derived registered type)",
            dclass->GetName());
        return nullptr;
    }
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

    if (m_activeScene)
        m_activeScene->RemoveGameObject(gameObject);

    auto it = std::find(m_gameObjects.begin(), m_gameObjects.end(), gameObject);
    if (it == m_gameObjects.end())
        return;

    GameObject* obj = *it;

    if (SceneComponent* rootSC = obj->GetRootSceneComponent())
        rootSC->SetParent(nullptr);

    obj->m_currentWorld = nullptr;
    obj->Destroy();

    if (obj->HasOwningAsset())
        obj->GetOwningAsset()->RemoveObject(obj->GetObjectId());

    m_gameObjects.erase(it);
    m_gameObjectsChanged = true;
}

void DeltaEngine::DWorld::InitRenderers(std::shared_ptr<DXGraphicsContext> context) const
{
    if (!DELTA_ENSURE(context != nullptr))
        return;

    DELTA_ASSERT(m_rootSceneComponent != nullptr);

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
        m_skybox->Initialize(context);
}

void DWorld::PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) const
{
    if (!DELTA_ENSURE(context != nullptr))
        return;

    DELTA_ASSERT(m_rootSceneComponent != nullptr);

    context->directionalLights.clear();
    context->pointLights.clear();
    context->spotLights.clear();

    std::stack<SceneComponent*> stack;
    stack.push(m_rootSceneComponent);
    bool hasCamera = false;
    int cameraBindCount = 0;

    while (!stack.empty())
    {
        SceneComponent* current = stack.top();
        stack.pop();

        if (Camera* camProbe = dynamic_cast<Camera*>(current))
            ++cameraBindCount;

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

    if (cameraBindCount > 1)
        DLOG(LogCore, ELogLevel::Warning,
            "PreGatherDrawCalls: {} Camera scene components found under root; only the first binds renderContext->camera (expected 1)",
            cameraBindCount);
}

void DWorld::GatherOpaqueDrawCalls(std::shared_ptr<DXGraphicsContext> context) const
{
    if (!DELTA_ENSURE(context != nullptr))
        return;

    DELTA_ASSERT(m_rootSceneComponent != nullptr);

    ForEachRenderer(m_rootSceneComponent, [&](Renderer* renderer)
    {
        renderer->GatherDrawCalls(context);
    });
}

void DWorld::GatherTransparentDrawCalls(std::shared_ptr<DXGraphicsContext> context) const
{
    if (!DELTA_ENSURE(context != nullptr))
        return;

    DELTA_ASSERT(m_rootSceneComponent != nullptr);

    XMVECTOR cameraPosition = XMVectorZero();
    if (context->activeRenderCamera.has_value())
        cameraPosition = context->activeRenderCamera->cb.position;

    std::vector<TransparentDrawEntry> entries;
    ForEachRenderer(m_rootSceneComponent, [&](Renderer* renderer)
    {
        if (MeshRenderer* meshRenderer = dynamic_cast<MeshRenderer*>(renderer))
            meshRenderer->CollectTransparentDrawEntries(entries, cameraPosition);
    });

    SortTransparentDrawEntriesDescending(entries);

    const ScenePassType previousPass = context->activePass;
    context->activePass = ScenePassType::Transparent;

    for (const TransparentDrawEntry& entry : entries)
    {
        if (entry.proxy)
            entry.proxy->DrawSubmesh(context, entry.submeshIndex);
    }

    context->activePass = previousPass;
}

void DWorld::GatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) const
{
    GatherOpaqueDrawCalls(context);

    // Skybox draws last: LESS_EQUAL depth test lets it fill pixels the scene didn't touch.
    if (m_skybox)
        m_skybox->GatherDrawCalls(context);
}

void DWorld::GatherShadowViews(std::shared_ptr<DXGraphicsContext> context, std::vector<ShadowView>& outViews) const
{
    if (!DELTA_ENSURE(context != nullptr))
        return;

    DELTA_ASSERT(m_rootSceneComponent != nullptr);

    std::stack<SceneComponent*> stack;
    stack.push(m_rootSceneComponent);

    while (!stack.empty())
    {
        SceneComponent* current = stack.top();
        stack.pop();

        if (LightComponent* light = dynamic_cast<LightComponent*>(current))
        {
            if (RenderProxy* proxy = light->GetRenderProxy())
                proxy->GatherShadowViews(context, outViews);
        }

        size_t childCount = current->m_children.size();
        for (int i = static_cast<int>(childCount) - 1; i >= 0; --i)
        {
            SceneComponent* child = current->m_children[i];
            stack.push(child);
        }
    }
}

void DWorld::GatherShadowDrawCalls(std::shared_ptr<DXGraphicsContext> context, const ShadowView& view) const
{
    if (!DELTA_ENSURE(context != nullptr))
        return;

    DELTA_ASSERT(m_rootSceneComponent != nullptr);

    std::stack<SceneComponent*> stack;
    stack.push(m_rootSceneComponent);

    while (!stack.empty())
    {
        SceneComponent* current = stack.top();
        stack.pop();

        if (Renderer* renderer = dynamic_cast<Renderer*>(current))
            renderer->GatherShadowDrawCalls(context, view);

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
    (void)deltaTime;
    m_gameObjectsChanged = false;
}

void DeltaEngine::DWorld::Clear()
{
    while (!m_gameObjects.empty())
        DestroyGameObject(m_gameObjects.front());

    // m_skybox is borrowed from the active DScene (which references it cross-asset
    // by {assetId, objectId} and is owned by PA_Skybox). The asset database owns
    // its lifetime — null the pointer here but do not destroy the object.
    m_skybox = nullptr;

    m_activeScene = nullptr;
}

void DWorld::DestroyAllWorldGameObjects()
{
    while (!m_gameObjects.empty())
        DestroyGameObject(m_gameObjects.front());
}

void DWorld::DetachAllWorldGameObjects()
{
    SetActiveScene(nullptr);

    for (GameObject* go : m_gameObjects)
    {
        go->m_currentWorld = nullptr;
        if (SceneComponent* rootSC = go->GetRootSceneComponent())
            rootSC->SetParent(nullptr);
    }
    m_gameObjects.clear();
    m_gameObjectsChanged = true;
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

    GameObject* go = GetReflectionRegistry().CreateObject<GameObject>(dclass->GetName());
    if (!go)
    {
        DLOG(LogCore, ELogLevel::Error,
            "CreateGameObjectInScene: reflection CreateObject<GameObject> failed for class '{}' (expected registered GameObject-derived type)",
            dclass->GetName());
        return nullptr;
    }

    go->m_name = std::format("New {}", dclass->GetName());
    go->m_currentWorld = this;

    if (!scene)
    {
        go->SetObjectId(UUID::Generate());
        m_gameObjects.push_back(go);
        m_gameObjectsChanged = true;
        return go;
    }

    DPrimaryAsset* pa = scene->GetOwningAsset();
    go->SetObjectId(UUID::Generate());

    if (pa)
        pa->AddObject(go);

    scene->AddGameObject(go);

    m_gameObjects.push_back(go);
    m_gameObjectsChanged = true;

    return go;
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
