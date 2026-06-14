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
#include "Graphics/Renderer/MeshRenderer.h"
#include "Graphics/Renderer/Renderer.h"
#include "Graphics/RenderProxy/CameraRenderProxy.h"
#include "Graphics/DirectX/CommandList.h"
#include "IO/IOManager.h"
#include "Reflection/ReflectionRegistry.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GC/GCManager.h"
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
    LoggingManager::Initialize(IOManager::GetIntermediateFolder() / "Logs");
    time = std::make_unique<Time>();
    GetReflectionRegistry().FinalizeRegistration();
    DLOG(LogEngine, ELogLevel::Display, "EngineMain constructed");
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

GCManager& EngineMain::GetGCManager() const
{
    return DeltaEngine::GetGCManager();
}

void EngineMain::TickGC()
{
    GCManager& gc = DeltaEngine::GetGCManager();
    gc.RequestCollect();
    gc.Tick();
}

void EngineMain::Initialize(std::shared_ptr<DXRenderManager> sceneRenderer)
{
    if (!DELTA_ENSURE_MSG(sceneRenderer != nullptr, "EngineMain::Initialize received null scene renderer"))
        return;

    DLOG(LogEngine, ELogLevel::Display, "EngineMain::Initialize: scene renderer={}",
        static_cast<void*>(sceneRenderer.get()));

    dxRenderManager = std::move(sceneRenderer);
}

void EngineMain::PreTick()
{
    if (DWorld* world = GetWorld())
        world->PreTick(Time::deltaTime);
}

void EngineMain::Tick()
{
    DELTA_ENSURE_MSG(time != nullptr, "EngineMain::Tick called before Time was constructed");
    if (!time)
        return;
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
    {
        DLOG(LogEngine, ELogLevel::Warning,
            "EngineMain::OnWindowResized ignored: height==0 (width={})", width);
        return;
    }

    DLOG(LogEngine, ELogLevel::Verbose, "EngineMain::OnWindowResized: {}x{}", width, height);

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
        if (context->activePass != ScenePassType::GBuffer)
            context->ApplyLightBuffersToCommandList();

        PIXBeginEvent(d3dCL, PIX_COLOR_DEFAULT, L"GatherDrawCalls");
        world->GatherOpaqueDrawCalls(context);
        PIXEndEvent(d3dCL);
    }
}

void EngineMain::CreateWorld()
{
    for (const WorldContext& ctx : m_worldContextList)
    {
        if (ctx.type == WorldType::Editor)
        {
            DLOG(LogEngine, ELogLevel::Warning,
                "EngineMain::CreateWorld called twice; existing editor world {} retained",
                static_cast<void*>(ctx.world));
            return;
        }
    }

    DWorld* world = CreateDObject<DWorld>();
    if (!DELTA_ENSURE_MSG(world != nullptr, "EngineMain::CreateWorld: CreateDObject<DWorld> returned null"))
        return;

    m_worldContextList.push_back(WorldContext { WorldType::Editor, world });
    m_worldRoot = StrongDObjectPtr<DObject>(world);
    DLOG(LogEngine, ELogLevel::Display, "EngineMain::CreateWorld: editor world={}",
        static_cast<void*>(world));
}

void EngineMain::CreateGameObjects()
{
    DWorld* world = GetWorld();
    if (!DELTA_ENSURE_MSG(world != nullptr, "EngineMain::CreateGameObjects: no editor world"))
        return;
    IAssetDatabase& assetDb = AssetDatabaseLocator::Get();

    auto attachStaticMesh = [&](const char* goName, const std::filesystem::path& meshAsset, float x, float y, float z)
    {
        GameObject* go = world->CreateGameObjectInScene(m_worldContextList[0].world->GetActiveScene(), goName);
        MeshRenderer* meshRenderer = go->AddSceneComponent<MeshRenderer>();
        meshRenderer->SetLocalPosition(x, y, z);

        const AssetId id = assetDb.FindAssetIdByPath(IOManager::GetEngineImportedAssetFullPath(meshAsset));
        PA_StaticMesh* paStaticMesh = assetDb.LoadAsset<PA_StaticMesh>(id);
        if (!paStaticMesh)
        {
            DLOG(LogEngine, ELogLevel::Warning,
                "EngineMain::CreateGameObjects: failed to load static mesh asset '{}' for GameObject '{}'",
                meshAsset.string(), goName);
            return;
        }
        meshRenderer->SetMesh(paStaticMesh->GetStaticMesh());
    };

    attachStaticMesh("MeshRenderer", "StarMesh", 2.0f, 0.0f, 5.0f);
    attachStaticMesh("HomeMeshRenderer", "HomeMesh", 0.0f, 0.0f, 0.0f);

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
        world->DetachAllWorldGameObjects();
    else
    {
        DLOG(LogEngine, ELogLevel::Error,
            "EngineMain::LoadScene: no editor world available (assetId={})",
            sceneAssetId.ToString());
        return;
    }

    DPrimaryAsset* asset = AssetDatabaseLocator::Get().LoadAsset(sceneAssetId);
    if (!asset)
    {
        DLOG(LogEngine, ELogLevel::Warning,
            "EngineMain::LoadScene: asset not found (assetId={})",
            sceneAssetId.ToString());
        return;
    }

    PA_DScene* sceneAsset = dynamic_cast<PA_DScene*>(asset);
    if (!sceneAsset)
    {
        DLOG(LogEngine, ELogLevel::Error,
            "EngineMain::LoadScene: asset {} is not a PA_DScene",
            sceneAssetId.ToString());
        return;
    }

    DScene* scene = sceneAsset->GetScene();
    if (!scene)
    {
        DLOG(LogEngine, ELogLevel::Error,
            "EngineMain::LoadScene: PA_DScene {} has null inner DScene",
            sceneAssetId.ToString());
        return;
    }

    for (GameObject* go : scene->GetGameObjects())
        world->AddGameObjectFromScene(go);

    world->SetActiveScene(scene);

    DLOG(LogEngine, ELogLevel::Display,
        "EngineMain::LoadScene: loaded scene {} ({} game objects)",
        sceneAssetId.ToString(), scene->GetGameObjects().size());

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
    if (m_worldContextList.empty() && !dxRenderManager)
        return;

    DLOG(LogEngine, ELogLevel::Display, "EngineMain::Cleanup");

    m_cameraGameObject = nullptr;

    if (DWorld* world = GetWorld())
        world->Clear();

    m_worldRoot.Reset();
    m_worldContextList.clear();

    if (dxRenderManager)
    {
        dxRenderManager->OnDestroy();
        dxRenderManager.reset();
    }
}
