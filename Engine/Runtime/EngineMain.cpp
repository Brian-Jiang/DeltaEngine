#include "EngineMain.h"

#include "Assets/AssetDatabaseLocator.h"
#include "Assets/DPrimaryAsset.h"
#include "Assets/PA_CommonAssets.h"
#include "Assets/PA_DScene.h"
#include "Core/Camera.h"
#include "Core/DScene.h"
#include "Core/Time.h"
#include "Graphics/DXGraphicsContext.h"
#include "Graphics/DXRenderManager.h"
#include "Graphics/Light/DirectionalLight.h"
#include "Graphics/Light/PointLight.h"
#include "Graphics/Light/SpotLight.h"
#include "Graphics/PostProcess/PostProcessStack.h"
#include "Graphics/Renderer/MeshRenderer.h"
#include "Graphics/Renderer/Renderer.h"
#include "Graphics/RenderProxy/CameraRenderProxy.h"
#include "Graphics/DirectX/CommandList.h"
#include "IO/IOManager.h"
#include "Reflection/ReflectionRegistry.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Logging/LoggingManager.h"

#include <SDL3/SDL.h>
#include <pix3.h>

namespace
{
constexpr UINT kSceneWidth = 1280;
constexpr UINT kSceneHeight = 720;
}

using namespace DeltaEngine;
using namespace DirectX;

EngineMain::EngineMain()
{
    LoggingManager::Initialize(IOManager::GetIntermediateFolder() + "Logs/");
    time = std::make_unique<Time>();
    GetReflectionRegistry().FinalizeRegistration();
}

EngineMain::~EngineMain() = default;

DWorld* EngineMain::GetWorld() const
{
    for (const WorldContext& ctx : m_worldContextList)
    {
        if (ctx.type == WorldType::Editor && ctx.world)
            return ctx.world;
    }

    return nullptr;
}

void EngineMain::Initialize(std::shared_ptr<DXRenderManager> sceneRenderer)
{
    DLOG(LogCore, ELogLevel::Display, "Initializing EngineMain with scene renderer {}.", static_cast<void*>(sceneRenderer.get()));

    dxRenderManager = std::move(sceneRenderer);
}

void EngineMain::PreTick()
{
    if (DWorld* world = GetWorld())
        world->PreTick(Time::deltaTime);
}

void EngineMain::Tick()
{
    time->TickTime();
}

Camera* EngineMain::GetCamera()
{
    if (!m_cameraGameObject)
        return nullptr;

    return m_cameraGameObject->GetRootSceneComponent<Camera>();
}

void EngineMain::ProcessEvent(const SDL_Event& event)
{
    (void)event;
}

void EngineMain::OnWindowResized(UINT width, UINT height)
{
    if (height == 0)
        return;

    if (Camera* camera = GetCamera())
        camera->UpdateAspectRatio(static_cast<float>(width) / static_cast<float>(height));
}

void EngineMain::RecordSceneDraws(std::shared_ptr<DXGraphicsContext> context)
{
    if (DWorld* world = GetWorld())
    {
        auto* d3dCL = context->commandList->GetD3D12CommandList().Get();

        if (context->activeRenderCamera.has_value())
            context->commandList->SetGraphicsDynamicConstantBuffer(0, context->activeRenderCamera->cb);
        context->ApplyLightBuffersToCommandList();

        PIXBeginEvent(d3dCL, PIX_COLOR_DEFAULT, L"GatherDrawCalls");
        world->GatherDrawCalls(context);
        PIXEndEvent(d3dCL);

        PostProcessStack* stack = context->camera ? context->camera->GetPostProcessStack() : nullptr;
        if (stack && stack->GetPassCount() > 0)
        {
            // TODO Phase 3: execute passes
        }
    }
}

void EngineMain::CreateWorld()
{
    DWorld* world = CreateDObject<DWorld>();
    m_worldContextList.push_back(WorldContext { WorldType::Editor, world });
}

void EngineMain::CreateGameObjects()
{
    DWorld* world = GetWorld();
    IAssetDatabase& assetDb = AssetDatabaseLocator::Get();

    {
        GameObject* go = world->CreateGameObjectInScene(m_worldContextList[0].world->GetActiveScene(), "MeshRenderer");

        MeshRenderer* meshRenderer = go->AddSceneComponent<MeshRenderer>();
        meshRenderer->SetLocalPosition(2.0f, 0.0f, 5.0f);

        PA_StaticMesh* paStaticMesh = assetDb.LoadAsset<PA_StaticMesh>(assetDb.FindAssetIdByPath(IOManager::GetEngineImportedAssetFullPath("StarMesh")));
        meshRenderer->SetMesh(paStaticMesh->GetStaticMesh());
    }

    {
        GameObject* go = world->CreateGameObjectInScene(m_worldContextList[0].world->GetActiveScene(), "HomeMeshRenderer");

        MeshRenderer* meshRenderer = go->AddSceneComponent<MeshRenderer>();
        meshRenderer->SetLocalPosition(0.0f, 0.0f, 0.0f);

        PA_StaticMesh* paStaticMesh = assetDb.LoadAsset<PA_StaticMesh>(assetDb.FindAssetIdByPath(IOManager::GetEngineImportedAssetFullPath("HomeMesh")));
        meshRenderer->SetMesh(paStaticMesh->GetStaticMesh());
    }

    GameObject* cameraGo = world->CreateGameObjectInScene(m_worldContextList[0].world->GetActiveScene(), "Camera");
    m_cameraGameObject = cameraGo;
    Camera* camera = cameraGo->AddSceneComponent<Camera>();
    camera->SetLocalPosition(0.0f, 50.0f, -400.0f);
    camera->UpdateParameters(XM_PIDIV4, static_cast<float>(kSceneWidth) / static_cast<float>(kSceneHeight), 0.1f, 1000.0f);

    GameObject* directionalLightGo = world->CreateGameObjectInScene(m_worldContextList[0].world->GetActiveScene(), "DirectionalLight");
    DirectionalLight* directionalLight = directionalLightGo->AddSceneComponent<DirectionalLight>();
    directionalLight->SetLocalRotation(DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(DirectX::SimpleMath::Vector3::UnitX, XM_PIDIV4));
    directionalLight->UpdateParameters(XMVectorSet(0, -1, 0, 0), XMVectorSet(1.0f, 1.0f, 0.95f, 1.0f), 0.7f);

    GameObject* pointLightGo = world->CreateGameObjectInScene(m_worldContextList[0].world->GetActiveScene(), "PointLight");
    PointLight* pointLight = pointLightGo->AddSceneComponent<PointLight>();
    pointLight->SetLocalPosition(0.0f, 3.0f, 2.0f);
    pointLight->UpdateParameters(XMVectorSet(1.0f, 0.4f, 0.2f, 1.0f), 2.0f, 15.0f);

    GameObject* spotLightGo = world->CreateGameObjectInScene(m_worldContextList[0].world->GetActiveScene(), "SpotLight");
    SpotLight* spotLight = spotLightGo->AddSceneComponent<SpotLight>();
    spotLight->SetLocalPosition(-2.0f, 4.0f, 0.0f);
    spotLight->SetLocalRotation(DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(DirectX::SimpleMath::Vector3::UnitZ, -XM_PIDIV4));
    spotLight->UpdateParameters(XMVectorSet(0.2f, 0.8f, 1.0f, 1.0f), 3.0f, 20.0f, XM_PI / 6.0f, XM_PI / 3.0f);

    if (dxRenderManager)
        dxRenderManager->InitWorldRenderers(*world);
}

void EngineMain::LoadScene(const AssetId& sceneAssetId)
{
    DWorld* world = GetWorld();
    if (world)
    {
        world->DestroyAllWorldGameObjects();
        world->SetActiveScene(nullptr);
    }

    DPrimaryAsset* asset = AssetDatabaseLocator::Get().LoadAsset(sceneAssetId);
    if (!asset)
        return;

    DScene* scene = nullptr;
    if (PA_DScene* sceneAsset = dynamic_cast<PA_DScene*>(asset))
        scene = sceneAsset->GetScene();

    if (!scene)
        return;

    if (!world)
        return;

    for (GameObject* go : scene->GetGameObjects())
        world->AddGameObjectFromScene(go);

    world->SetActiveScene(scene);

    if (dxRenderManager)
    {
        for (GameObject* go : scene->GetGameObjects())
        {
            if (Camera* cam = go->GetRootSceneComponent<Camera>())
            {
                m_cameraGameObject = go;
                cam->UpdateRenderProxy();
                break;
            }
        }

        for (GameObject* go : scene->GetGameObjects())
        {
            if (Renderer* renderer = go->GetRootSceneComponent<Renderer>())
                renderer->CreateRenderProxy();
        }

        dxRenderManager->InitWorldRenderers(*world);
    }
}

void EngineMain::Cleanup()
{
    m_cameraGameObject = nullptr;

    if (DWorld* world = GetWorld())
        world->Clear();

    m_worldContextList.clear();

    if (dxRenderManager)
    {
        dxRenderManager->OnDestroy();
        dxRenderManager.reset();
    }
}
